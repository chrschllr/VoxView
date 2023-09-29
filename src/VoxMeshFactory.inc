{interface}
{$ifdef INCLUDE_SECTION_INTERFACE}

type
  TTextureLoader = function(const Filename: String): TBGRABitmap is nested;

function MeshImportOBJ(const SourceOBJ: String; const SourceMTL: String = '';
                       TexLoader: TTextureLoader = Nil): TVoxMeshArray;
function MeshExportOBJ(const Mesh: TVoxMesh; const Name: String; ElementOffset: GLuint = 1): String;
function MeshImportBIN(const Source: TBytes): TVoxMesh;
function MeshExportBIN(const Mesh: TVoxMesh): TBytes;
function MeshCube(): TVoxMesh;
function MeshCircle(const Normal: TVec3; Steps: Integer = 36): TVoxMesh;
function MeshSphere(Steps: Integer = 3): TVoxMesh;
function MeshExtrude(const Axis: TVec3; Distance: Double; const Base: TVoxMesh): TVoxMesh;
function MeshLineStrip(const Points: array of TVec3; CloseLoop: Boolean = False): TVoxMesh;
function MeshPolygon(const Points: array of TVec3): TVoxMesh;
function MeshRevolve(const Pivot, Axis: TVec3; const Base: TVoxMesh; Steps: Integer = 64;
                     AngleDeg: Double = 360; CloseLoop: Boolean = True): TVoxMesh;
function MeshLabel(const VecA, VecB: TVec3; const Text: String; Height: Double; const Normal: TVec3;
                   const Color: TColor4; CanvasHeight: Integer = 256; Font: TBGRAVectorizedFont = Nil): TVoxMesh;
function MeshNormalSpikes(const Mesh: TVoxMesh; SpikeLength: Double = 0.1): TVoxMesh;

var
  VOX_FONT_LABEL_DEFAULT: TBGRAVectorizedFont;

{$endif}

{implementation}
{$ifdef INCLUDE_SECTION_IMPLEMENTATION}

{$r Fonts.rc}

type
  PRingVertex = ^TRingVertex;
  TRingVertex = record
    Prev, Next: PRingVertex;
    Element: GLuint;
    IsEar: Boolean;
  end;
  TVec2Buffer = specialize TArrayList<TVec2>;
  TVec3Buffer = specialize TArrayList<TVec3>;
  TColor4Buffer = specialize TArrayList<TColor4>;
  TElementBuffer = specialize TArrayList<GLuint>;
  TBGRABitmapList = specialize TArrayList<TBGRABitmap>;
  TMaterial = record
    Phong: TVoxPhongMaterial;
    Textures: TBGRABitmapList;
    class operator =(const MatA, MatB: TMaterial): Boolean;
  end;
  TMaterialMap = specialize TLinearMap<String, TMaterial>;
  TWavefrontTokens = array of array of String;

class operator TMaterial.=(const MatA, MatB: TMaterial): Boolean;
begin
  Result := CompareByte(MatA, MatB, SizeOf(MatA)) = 0;
end;

function ParseFloat(const Str: String): GLfloat;
begin
  Result := StrToFloat(Str, VOX_FORMAT);
end;

function ToWavefrontTokens(const Source: String): TWavefrontTokens;
var
  SourceLines: TStringArray;
  I: Integer;
begin
  Result := Default(TWavefrontTokens);                // TWavefrontTokens is a list of token arrays
  SourceLines := Source.Split(LineEnding);            // Split contiguos source string into lines first
  SetLength(Result, Length(SourceLines));
  for I := 0 to Length(Result) - 1 do begin
    Result[I] := SourceLines[I].Trim().Split(' ');    // Tokens are delimited by spaces
    SetLength(Result[I], Max(1, Length(Result[I])));  // Ensure at least 1 token per line (content doesn't matter)
  end;
end;

function ImportMTL(const Source: String; out Error: String; TexLoader: TTextureLoader): TMaterialMap;
var
  Tokens: TStringArray;
  TexSource: TBGRABitmap;
  MatName: String = '';
  I: Integer;
begin
  Result := TMaterialMap.Create();
  for Tokens in ToWavefrontTokens(Source) do try
    case Tokens[0] of
      'newmtl': begin
        MatName := Tokens[Length(Tokens) - 1];
        if not Result.FindKey(MatName, I) then begin
          I := Result.Put(MatName, Default(TMaterial));
          Result.Values[I].Phong := PHONG_MATERIAL_DEFAULT;
          Result.Values[I].Textures := TBGRABitmapList.Create();
        end;
      end;
      'Ka', 'Kd', 'Ks', 'Ns', 'Tr', 'd': if Result.FindKey(MatName, I) then begin
        with Result.Values[I].Phong do case Tokens[0] of
          'Ka': Ambient := Color4(ParseFloat(Tokens[1]), ParseFloat(Tokens[2]), ParseFloat(Tokens[3]), NaN);
          'Kd': Diffuse := Color4(ParseFloat(Tokens[1]), ParseFloat(Tokens[2]), ParseFloat(Tokens[3]), NaN);
          'Ks': Specular := Color4(ParseFloat(Tokens[1]), ParseFloat(Tokens[2]), ParseFloat(Tokens[3]), NaN);
          'Ns': Shininess := ParseFloat(Tokens[1]);
          'Tr': Alpha := 1.0 - ParseFloat(Tokens[1]);
          'd': Alpha := ParseFloat(Tokens[1]);
        end;
      end;
      'map_Ka', 'map_Kd', 'map_Ks': if (TexLoader <> Nil) and Result.FindKey(MatName, I) then begin
        TexSource := TexLoader(Tokens[Length(Tokens) - 1]);
        with Result.Values[I] do if TexSource <> Nil then Textures.Push(TexSource);
      end;
    end;
  except on Ex: Exception do Error := Ex.Message;
  end;
end;

function MeshImportOBJ(const SourceOBJ: String; const SourceMTL: String = '';
                       TexLoader: TTextureLoader = Nil): TVoxMeshArray;
var // May the powers of the universe ensure that there is enough stack space for this function to run
  SourceTokens: TWavefrontTokens;
  Tokens, SubTokens: TStringArray;
  PositionBuffer, NormalBuffer: TVec3Buffer;
  ColorBuffer: TColor4Buffer;
  TexCoordBuffer: TVec2Buffer;
  VertexBuffer: TVertexBuffer;
  ElementBuffer: TElementBuffer;
  MeshBuffer: TVoxMeshList;
  NewVertex: TVertex;
  PolySides, FaceFormat: Integer;
  ElementOffset: SizeInt;
  SmoothState: Boolean = False;
  SmoothStateBuffer: TBooleanBuffer;
  Materials: TMaterialMap;
  MatName: String = '';
  ImportError: String = '';
  I: Integer;
  procedure BreakMesh();
  var
    NewMesh: TVoxMesh;
    J, K: Integer;
  begin
    if ElementBuffer.Count > 0 then begin
      NewMesh := TVoxMesh.Create(VertexBuffer.Items, ElementBuffer.Items);
      if Materials.FindKey(MatName, J) then with Materials.Values[J] do begin
        for K := 0 to Textures.Count - 1 do NewMesh.LoadTexture(Textures[K]);
        NewMesh.Material := Phong;
      end;
      NewMesh.ShadeSmooth(NewMesh.Textures.Count > 0, SmoothStateBuffer.Items);
      NewMesh.Error := ImportError;
      MeshBuffer.Push(NewMesh);
    end;
    VertexBuffer.Clear();
    ElementBuffer.Clear();
    SmoothStateBuffer.Clear();
  end;
begin
  // Buffer initialization
  PositionBuffer := TVec3Buffer.Create();
  NormalBuffer := TVec3Buffer.Create();
  ColorBuffer := TColor4Buffer.Create();
  TexCoordBuffer := TVec2Buffer.Create();
  VertexBuffer := TVertexBuffer.Create();
  ElementBuffer := TElementBuffer.Create();
  SmoothStateBuffer := TBooleanBuffer.Create();
  MeshBuffer := TVoxMeshList.Create();
  Materials := ImportMTL(SourceMTL, ImportError, TexLoader);
  SourceTokens := ToWavefrontTokens(SourceOBJ);
  // Per-line iteration #1 (vertex attribute extraction)
  for Tokens in SourceTokens do try
    case Tokens[0] of
      'v': begin
        PositionBuffer.Push(Vec3(ParseFloat(Tokens[1]), ParseFloat(Tokens[2]), ParseFloat(Tokens[3])));
        if Length(Tokens) >= 7 then begin // Vertex has color data
          ColorBuffer.Push(Color4(ParseFloat(Tokens[4]), ParseFloat(Tokens[5]), ParseFloat(Tokens[6]), 1.0));
          if Length(Tokens) >= 8 then ColorBuffer.Last^.A := ParseFloat(Tokens[7]); // Color with alpha
        end else ColorBuffer.Push(COLOR4_WHITE);
      end;
      'vn': NormalBuffer.Push(Vec3(ParseFloat(Tokens[1]), ParseFloat(Tokens[2]), ParseFloat(Tokens[3])));
      'vt': TexCoordBuffer.Push(Vec2(ParseFloat(Tokens[1]), ParseFloat(Tokens[2])));
    end;
  except on Ex: Exception do ImportError := Ex.Message;
  end;
  // Per-line iteration #2 (object/triangle building)
  for Tokens in SourceTokens do try
    case Tokens[0] of
      'f': begin
        PolySides := Length(Tokens) - 1;
        for I := 1 to Length(Tokens) - 1 do begin // Parse each face vertex element
          SubTokens := Tokens[I].Split('/');
          FaceFormat := Length(SubTokens); // Either "v", "v/vt", "v/vt/vn" or "v//vn"
          SetLength(SubTokens, 3); // Assume texture coordinates and normals are given
          if FaceFormat < 3 then SubTokens[2] := ''; // "v/vt" -> vn = ''
          if FaceFormat < 2 then SubTokens[1] := ''; // "v"    -> vt = '', vn = ''
          NewVertex.Position := PositionBuffer[StrToInt(SubTokens[0]) - 1];
          NewVertex.Color := ColorBuffer[StrToInt(SubTokens[0]) - 1];
          if SubTokens[1].Trim().Length > 0 then begin
            NewVertex.TexCoord := TexCoordBuffer[StrToInt(SubTokens[1]) - 1];
          end else NewVertex.TexCoord := Vec2(NaN, NaN);
          if SubTokens[2].Trim().Length > 0 then begin
            NewVertex.Normal := NormalBuffer[StrToInt(SubTokens[2]) - 1];
          end else NewVertex.Normal := Vec3(NaN, NaN, NaN);
          VertexBuffer.Push(NewVertex);
        end;
        for I := 0 to PolySides - 3 do begin // Add triangle fan vertices and elements
          ElementOffset := VertexBuffer.Count - PolySides;
          ElementBuffer.Push([ElementOffset + 0, ElementOffset + I + 1, ElementOffset + I + 2]);
          SmoothStateBuffer.Push(SmoothState);
        end;
      end;
      'o': BreakMesh();
      's': SmoothState := (Tokens[1] = 'on') or (Tokens[1] = '1');
      'usemtl': MatName := Tokens[Length(Tokens) - 1];
    end;
  except on Ex: Exception do ImportError := Ex.Message;
  end;
  BreakMesh();
  Result := MeshBuffer.Items;
  // Buffer cleanup
  PositionBuffer.Free();
  NormalBuffer.Free();
  ColorBuffer.Free();
  TexCoordBuffer.Free();
  VertexBuffer.Free();
  ElementBuffer.Free();
  SmoothStateBuffer.Free();
  MeshBuffer.Free();
  for I := 0 to Materials.Count - 1 do begin
    Materials.Values[I].Textures.FreeItems(True);
  end;
  Materials.Free();
end;

function MeshExportOBJ(const Mesh: TVoxMesh; const Name: String; ElementOffset: GLuint = 1): String;
var
  Lines: TStringList;
  EmA, EmB, EmC: GLuint;
  I: SizeInt;
begin
  Lines := TStringList.Create();
  Lines.Add(Format('o %s', [Name]));
  for I := 0 to Length(Mesh.Vertices) - 1 do with Mesh.Vertices[I] do begin
    Lines.Add(Format('v %.6f %.6f %.6f %.3f %.3f %.3f %.3f', [
      Position.X, Position.Y, Position.Z,
      Color.R, Color.G, Color.B, Color.A
    ], VOX_FORMAT));
    Lines.Add(Format('vn %.6f %.6f %.6f', [Normal.X, Normal.Y, Normal.Z], VOX_FORMAT));
    Lines.Add(Format('vt %.6f %.6f', [TexCoord.X, TexCoord.Y], VOX_FORMAT));
  end;
  case Mesh.DrawType of
    GL_TRIANGLES: for I := 0 to (Length(Mesh.Elements) div 3) - 1 do begin
      EmA := Mesh.Elements[I * 3 + 0] + ElementOffset;
      EmB := Mesh.Elements[I * 3 + 1] + ElementOffset;
      EmC := Mesh.Elements[I * 3 + 2] + ElementOffset;
      Lines.Add(Format('f %u/%u/%u %u/%u/%u %u/%u/%u', [
        EmA, EmA, EmA, EmB, EmB, EmB, EmC, EmC, EmC
      ]));
    end;
    GL_LINES: for I := 0 to (Length(Mesh.Elements) div 2) - 1 do begin
      EmA := Mesh.Elements[I * 2 + 0] + ElementOffset;
      EmB := Mesh.Elements[I * 2 + 1] + ElementOffset;
      Lines.Add(Format('l %u %u', [EmA, EmB]));
    end;
  end;
  Result := Lines.Text;
  Lines.Free();
end;

function MeshImportBIN(const Source: TBytes): TVoxMesh;
var
  SourcePos: PByte;
  I: SizeInt;
  function ReadU32(): GLuint; inline;
  begin
    Result := BEtoN(PGLuint(SourcePos)^);
    SourcePos += SizeOf(Result);
  end;
  function ReadF32(): GLfloat; inline;
  begin
    PUInt32(@Result)^ := BEtoN(PUInt32(SourcePos)^);
    SourcePos += SizeOf(Result);
  end;
begin
  SourcePos := @Source[0];
  Result := TVoxMesh.Create();
  with Result do begin
    SetLength(Elements, ReadU32(){%H-});
    SetLength(Vertices, ReadU32());
    for I := 0 to Length(Elements) - 1 do Elements[I] := ReadU32();
    for I := 0 to Length(Vertices) - 1 do with Vertices[I] do begin
      Position.X := ReadF32();  Position.Y := ReadF32();  Position.Z := ReadF32();
      Normal.X := ReadF32();    Normal.Y := ReadF32();    Normal.Z := ReadF32();
      TexCoord.X := ReadF32();  TexCoord.Y := ReadF32();
      Color.R := ReadF32();     Color.G := ReadF32();     Color.B := ReadF32();     Color.A := ReadF32();
    end;
  end;
end;

function MeshExportBIN(const Mesh: TVoxMesh): TBytes;
var
  DestPos: PByte;
  I: SizeInt;
  procedure WriteU32(Value: GLuint); inline;
  begin
    PGLuint(DestPos)^ := NtoBE(Value);
    DestPos += SizeOf(Value);
  end;
  procedure WriteF32(Value: GLfloat); inline;
  begin
    PUInt32(DestPos)^ := NtoBE(PUInt32(@Value)^);
    DestPos += SizeOf(Value);
  end;
begin
  Result := Default(TBytes);
  with Mesh do begin
    SetLength(Result,
      SizeOf(GLuint) * 2 + Length(Elements) * SizeOf(Elements[0]) + Length(Vertices) * SizeOf(Vertices[0])
    );
    DestPos := @Result[0];
    {%H-}WriteU32(Length(Elements));
    WriteU32(Length(Vertices));
    for I := 0 to Length(Elements) - 1 do WriteU32(Elements[I]);
    for I := 0 to Length(Vertices) - 1 do with Vertices[I] do begin
      WriteF32(Position.X); WriteF32(Position.Y); WriteF32(Position.Z);
      WriteF32(Normal.X);   WriteF32(Normal.Y);   WriteF32(Normal.Z);
      WriteF32(TexCoord.X); WriteF32(TexCoord.Y);
      WriteF32(Color.R);    WriteF32(Color.G);    WriteF32(Color.B);  WriteF32(Color.A);
    end;
  end;
end;

function MeshCube(): TVoxMesh;
const
  CUBE_TRIANGLE_POINTS: array of TVec3 = (
    (X:  1; Y:  1; Z: -1), (X:  1; Y: -1; Z: -1), (X: -1; Y: -1; Z: -1),
    (X:  1; Y:  1; Z: -1), (X: -1; Y: -1; Z: -1), (X: -1; Y:  1; Z: -1),
    (X: -1; Y: -1; Z: -1), (X:  1; Y: -1; Z: -1), (X:  1; Y: -1; Z:  1),
    (X: -1; Y: -1; Z: -1), (X:  1; Y: -1; Z:  1), (X: -1; Y: -1; Z:  1),
    (X: -1; Y:  1; Z: -1), (X: -1; Y: -1; Z: -1), (X: -1; Y: -1; Z:  1),
    (X: -1; Y:  1; Z: -1), (X: -1; Y: -1; Z:  1), (X: -1; Y:  1; Z:  1),
    (X:  1; Y:  1; Z: -1), (X: -1; Y:  1; Z: -1), (X: -1; Y:  1; Z:  1),
    (X:  1; Y:  1; Z: -1), (X: -1; Y:  1; Z:  1), (X:  1; Y:  1; Z:  1),
    (X:  1; Y: -1; Z: -1), (X:  1; Y:  1; Z: -1), (X:  1; Y:  1; Z:  1),
    (X:  1; Y: -1; Z: -1), (X:  1; Y:  1; Z:  1), (X:  1; Y: -1; Z:  1),
    (X: -1; Y:  1; Z:  1), (X: -1; Y: -1; Z:  1), (X:  1; Y: -1; Z:  1),
    (X: -1; Y:  1; Z:  1), (X:  1; Y: -1; Z:  1), (X:  1; Y:  1; Z:  1)
  );
begin
  Result := TVoxMesh.Create(CUBE_TRIANGLE_POINTS);
  Result.SetColor(COLOR4_WHITE);
end;

function MeshCircle(const Normal: TVec3; Steps: Integer = 36): TVoxMesh;
var
  Points: array of TVec3;
  StepMatrix: TMatrix4;
  I: Integer;
begin
  SetLength(Points{%H-}, Steps);
  StepMatrix := RotationMatrix(Normal, 2.0 * Pi() / Steps);
  Points[0] := Normal.RightAngled().Normal();
  for I := 1 to Steps - 1 do begin
    Points[I] := StepMatrix * Points[I - 1];
  end;
  Result := MeshLineStrip(Points, True);
end;

function MeshSphere(Steps: Integer = 3): TVoxMesh;
var
  I: Integer;
begin
  Result := MeshCube();
  Result.Subdivide(Steps);
  for I := 0 to Length(Result.Vertices) - 1 do begin
    Result.Vertices[I].Position := Result.Vertices[I].Position.Normal();
    Result.Vertices[I].Normal := Default(TVec3); // Normals need to be uniform so ShadeSmooth can merge the corners
  end;
  Result.ShadeSmooth();
  Result.RecalculateNormals();
end;

function MeshLineStrip(const Points: array of TVec3; CloseLoop: Boolean = False): TVoxMesh;
var
  LineCount: Integer;
  I: Integer;
begin
  Result := TVoxMesh.Create();
  LineCount := IfThen(CloseLoop, Length(Points), Length(Points) - 1);
  SetLength(Result.Vertices, Length(Points));
  SetLength(Result.Elements, LineCount * 2);
  for I := 0 to Length(Points) - 1 do Result.Vertices[I] := Vertex(Points[I]);
  for I := 0 to Length(Points) - 2 do begin
    Result.Elements[I * 2] := I;
    Result.Elements[I * 2 + 1] := I + 1;
  end;
  if CloseLoop then begin
    Result.Elements[Length(Result.Elements) - 2] := Length(Result.Vertices) - 1;
    Result.Elements[Length(Result.Elements) - 1] := 0;
  end;
  Result.DrawType := GL_LINES;
end;

function MeshExtrude(const Axis: TVec3; Distance: Double; const Base: TVoxMesh): TVoxMesh;
var
  LineVertCount, LineCount: Integer;
  QuadTL, QuadBL, QuadTR, QuadBR: Integer;
  I, J, Prev, Curr, JP1_MOD_LVC: Integer;
begin
  Result := TVoxMesh.Create(Base.Vertices, Base.Elements);
  // Prepare vertex and element buffers
  LineVertCount := Length(Base.Vertices);
  LineCount := Length(Base.Elements) div 2;
  SetLength(Result.Vertices, LineVertCount * 2);
  SetLength(Result.Elements, LineCount * 6);
  // Extrude vertices along "Axis"
  for I := 0 to LineVertCount - 1 do begin
    Curr := LineVertCount + I;
    Prev := I;
    Result.Vertices[Curr].Position := Result.Vertices[Prev].Position + (Axis * Distance);
    Result.Vertices[Curr].Color := Result.Vertices[Prev].Color;
  end;
  // Build "Steps" - 1 quad strips composed of "LineCount" quads each
  J := 0; // Element index, incremented after each quad
  for I := 0 to LineCount - 1 do begin
    JP1_MOD_LVC := I + 1; // (I + 1) mod LineVertCount (optimizable because "I" can at most be exactly LineVertCount)
    if JP1_MOD_LVC >= LineVertCount then JP1_MOD_LVC := 0;
    QuadTR := I + LineVertCount;
    QuadBR := QuadTR - I + JP1_MOD_LVC;
    QuadTL := I;
    QuadBL := QuadTL - I + JP1_MOD_LVC;
    Result.Elements[J + 0] := QuadTL;
    Result.Elements[J + 1] := QuadBR;
    Result.Elements[J + 2] := QuadTR;
    Result.Elements[J + 3] := QuadTL;
    Result.Elements[J + 4] := QuadBL;
    Result.Elements[J + 5] := QuadBR;
    J += 6; // One quad = two triangles -> 6 elements
  end;
  Result.ShadeFlat();
  Result.RecalculateNormals();
end;

function MeshPolygon(const Points: array of TVec3): TVoxMesh;
var
  PolyCount: SizeInt;
  FlatPoints: array of TVec2;
  Ring: array of TRingVertex;
  RV: PRingVertex;
  Normal: TVec3;
  ElementIndex: Integer = 0;
  RingCounter, RingLimit: Integer;
  I: Integer;
  procedure FlattenPoints();
  var
    NewX, NewY, NewZ: TVec3;
    ProjectionMatrix: TMatrix4;
    I: Integer;
  begin
    SetLength(FlatPoints{%H-}, PolyCount);
    NewX := Normal.Cross(Points[1] - Points[0]).Normal();
    NewY := Normal.Cross(NewX).Normal();
    NewZ := NewX.Cross(NewY).Normal();
    ProjectionMatrix := ReorientationMatrix(NewX, NewY, NewZ);
    for I := 0 to PolyCount - 1 do begin
      FlatPoints[I] := Vec2(ProjectionMatrix * Points[I]);
    end;
  end;
  function IsInTriangle(const P, V1, V2, V3: TVec2): Boolean;
  var
    B1, B2, B3: Boolean;
  begin
    B1 := (P - V1).Dot(Vec2(V1.Y - V2.Y, V2.X - V1.X)) > 0.0;
    B2 := (P - V2).Dot(Vec2(V2.Y - V3.Y, V3.X - V2.X)) > 0.0;
    B3 := (P - V3).Dot(Vec2(V3.Y - V1.Y, V1.X - V3.X)) > 0.0;
    Result := (B1 = B2) and (B2 = B3);
  end;
  function IsConvex(const RV: PRingVertex): Boolean;
  var
    VecTo, VecFrom: TVec3;
  begin
    VecTo := Points[RV^.Element] - Points[RV^.Prev^.Element];
    VecFrom := Points[RV^.Next^.Element] - Points[RV^.Element];
    Result := VecTo.Cross(VecFrom).Dot(Normal) > 0.0;
  end;
  function IsEar(const RV: PRingVertex): Boolean;
  var
    I: Integer;
    E1, E2, E3: SizeInt;
  begin
    Result := True;
    if not IsConvex(RV) then Exit(False);
    E1 := RV^.Element; E2 := RV^.Next^.Element; E3 := RV^.Prev^.Element;
    for I := 0 to PolyCount - 1 do begin
      if (I = E1) or (I = E2) or (I = E3) then continue;
      if IsInTriangle(FlatPoints[I], FlatPoints[E1], FlatPoints[E2], FlatPoints[E3]) then Exit(False);
    end;
  end;
  function FindRingNormal(const RV: PRingVertex): TVec3;
  var
    CurrentRV: PRingVertex;
    Vx0, LowCCW, HighCCW: TVec3;
  begin
    Result := Vec3(0.0, 0.0, 0.0);
    CurrentRV := RV;
    Vx0 := Points[RV^.Element];
    repeat
      LowCCW := Points[CurrentRV^.Element] - Vx0;
      HighCCW := Points[CurrentRV^.Next^.Element] - Vx0;
      Result += LowCCW.Cross(HighCCW);
      CurrentRV := CurrentRV^.Next;
    until CurrentRV = RV;
  end;
begin
  PolyCount := Length(Points);
  Result := TVoxMesh.Create();
  if PolyCount < 3 then Exit();
  SetLength(Result.Vertices{%H-}, PolyCount);
  SetLength(Result.Elements{%H-}, 3 * (PolyCount - 2));
  // Transform points to vertices
  for I := 0 to PolyCount - 1 do Result.Vertices[I] := Vertex(Points[I]);
  // Build vertex ring
  SetLength(Ring{%H-}, PolyCount);
  for I := 0 to PolyCount - 1 do begin
    Ring[I].Element := I;
    Ring[I].Prev := @Ring[(I + PolyCount - 1) mod PolyCount];
    Ring[I].Next := @Ring[(I + PolyCount + 1) mod PolyCount];
  end;
  // Project points onto 2D plane for ear checking
  Normal := FindRingNormal(@Ring[0]);
  FlattenPoints();
  for I := 0 to PolyCount - 1 do Ring[I].IsEar := IsEar(@Ring[I]);
  // Iterate over ring, removing ears, until there are no triangles left
  RV := @Ring[0];
  RingCounter := 0;
  RingLimit := Trunc(Power(Length(Points), 2) * 2);
  repeat
    if RV^.IsEar then begin
      // Add triangle
      Result.Elements[ElementIndex] := RV^.Element;
      Result.Elements[ElementIndex + 1] := RV^.Next^.Element;
      Result.Elements[ElementIndex + 2] := RV^.Prev^.Element;
      ElementIndex += 3;
      // Remove vertex from ring and recalculate adjacent ear status
      RV^.Prev^.Next := RV^.Next;
      RV^.Next^.Prev := RV^.Prev;
      RV^.Next^.IsEar := IsEar(RV^.Next);
      RV^.Prev^.IsEar := IsEar(RV^.Prev);
    end;
    RV := RV^.Next;
    RingCounter += 1;
  until (RV^.Prev = RV^.Next) or (RingCounter = RingLimit);
  SetLength(Result.Elements{%H-}, ElementIndex);
  Result.RecalculateNormals();
end;

function MeshRevolve(const Pivot, Axis: TVec3; const Base: TVoxMesh; Steps: Integer = 64;
                     AngleDeg: Double = 360; CloseLoop: Boolean = True): TVoxMesh;
var
  LineVertCount, LineCount, QuadCount: Integer;
  StepMatrix: TMatrix4;
  AngleStep: Double;
  QuadTL, QuadBL, QuadTR, QuadBR: Integer;
  I, J, K, Prev, Curr, JP1_MOD_LVC: Integer;
begin
  Result := TVoxMesh.Create(Base.Vertices, Base.Elements);
  // Prepare vertex and element buffers
  LineVertCount := Length(Base.Vertices);
  LineCount := Length(Base.Elements) div 2;
  QuadCount := LineCount * (Steps - 1);
  if CloseLoop then QuadCount += LineCount; // Closing the loop adds another quad strip
  SetLength(Result.Vertices, LineVertCount * Steps);
  SetLength(Result.Elements, QuadCount * 6);
  // Revolve vertices around "Axis" @ "Pivot"
  AngleStep := DegToRad(AngleDeg / IfThen(CloseLoop, Steps, Steps - 1));
  StepMatrix := TransformationMatrix([
    TranslationMatrix(-Pivot),
    RotationMatrix(Axis, AngleStep),
    TranslationMatrix(Pivot)
  ]);
  for I := 1 to Steps - 1 do begin
    for J := 0 to LineVertCount - 1 do begin
      Curr := J + I * LineVertCount;
      Prev := Curr - LineVertCount;
      Result.Vertices[Curr].Position := StepMatrix * Result.Vertices[Prev].Position;
      Result.Vertices[Curr].Color := Result.Vertices[Prev].Color;
    end;
  end;
  // Build "Steps" - 1 quad strips composed of "LineCount" quads each
  K := 0; // Element index, incremented after each quad
  for I := 0 to Steps - 2 do begin
    for J := 0 to LineCount - 1 do begin
      JP1_MOD_LVC := J + 1; // (J + 1) mod LineVertCount (optimizable because J can at most be exactly LineVertCount)
      if JP1_MOD_LVC >= LineVertCount then JP1_MOD_LVC := 0;
      QuadTR := J + I * LineVertCount;
      QuadBR := QuadTR - J + JP1_MOD_LVC;
      QuadTL := J + (I + 1) * LineVertCount;
      QuadBL := QuadTL - J + JP1_MOD_LVC;
      Result.Elements[K + 0] := QuadTL;
      Result.Elements[K + 1] := QuadBR;
      Result.Elements[K + 2] := QuadTR;
      Result.Elements[K + 3] := QuadTL;
      Result.Elements[K + 4] := QuadBL;
      Result.Elements[K + 5] := QuadBR;
      K += 6; // One quad = two triangles -> 6 elements
    end;
  end;
  // If specified, close the remaining gap with another quad strip
  if CloseLoop then begin
    for J := 0 to LineCount - 1 do begin
      JP1_MOD_LVC := J + 1;
      if JP1_MOD_LVC >= LineVertCount then JP1_MOD_LVC := 0;
      QuadTR := J + Length(Result.Vertices) - LineVertCount;
      QuadBR := QuadTR - J + JP1_MOD_LVC;
      QuadTL := J;
      QuadBL := JP1_MOD_LVC;
      Result.Elements[K + 0] := QuadTL;
      Result.Elements[K + 1] := QuadBR;
      Result.Elements[K + 2] := QuadTR;
      Result.Elements[K + 3] := QuadTL;
      Result.Elements[K + 4] := QuadBL;
      Result.Elements[K + 5] := QuadBR;
      K += 6;
    end;
  end;
  Result.RecalculateNormals();
end;

function MeshLabel(const VecA, VecB: TVec3; const Text: String; Height: Double; const Normal: TVec3;
                   const Color: TColor4; CanvasHeight: Integer = 256; Font: TBGRAVectorizedFont = Nil): TVoxMesh;
var
  BannerVertices: array[0..3] of TVertex;
  BannerWidth, BannerHeight: Double;
  CanvasWidth: Integer;
  LocalX, LocalY: TVec3;
  Bitmap: TBGRABitmap;
  TextColor, BackgroundColor: TBGRAPixel;
  TextSize: TPointF;
  I: Integer;
begin
  BannerWidth := VecA.DistTo(VecB);
  BannerHeight := Height;
  LocalX := (VecB - VecA).Normal();
  LocalY := Normal.Cross(LocalX).Normal();
  // Build quad from start/end points + height              // 3 *---*:::* 2          Y
  BannerVertices[0].Position := VecA;                       //   |  /   /|            ^
  BannerVertices[1].Position := VecB;                       //   | /   / | Height     |          Local space
  BannerVertices[2].Position := VecB + LocalY * Height;     //   |/   /  |            |
  BannerVertices[3].Position := VecA + LocalY * Height;     // 0 *:::*---* 1          *-----> X
  // Set texture coordinates                                //   VecA    VecB
  BannerVertices[0].TexCoord := Vec2(0.0, 0.0);
  BannerVertices[1].TexCoord := Vec2(1.0, 0.0);
  BannerVertices[2].TexCoord := Vec2(1.0, 1.0);
  BannerVertices[3].TexCoord := Vec2(0.0, 1.0);
  // Set normals and color
  for I := 0 to Length(BannerVertices) - 1 do begin
    BannerVertices[I].Normal := Normal;
    BannerVertices[I].Color := Color4(Color, 1.0);
  end;
  // Create canvas/banner texture and set colors
  CanvasWidth := Trunc(CanvasHeight * BannerWidth / BannerHeight);
  TextColor := BGRA(Trunc(Color.R * 255), Trunc(Color.G * 255), Trunc(Color.B * 255), Trunc(Color.A * 255));
  BackgroundColor := TextColor;
  BackgroundColor.Alpha := 0;
  Bitmap := TBGRABitmap.Create(CanvasWidth, CanvasHeight, BackgroundColor);
  Bitmap.Canvas2D.FillStyle(TextColor);
  // Set up font and draw text onto canvas
  if Font = Nil then Font := VOX_FONT_LABEL_DEFAULT;
  Font.Resolution := CanvasHeight * 2;
  TextSize := Font.GetTextSize(Text);
  Font.FullHeight := Min(CanvasHeight, Trunc(Font.FullHeight * CanvasWidth / TextSize.X));
  Font.DrawTextRect(Bitmap.Canvas2D, Text, 0, 0, CanvasWidth, CanvasHeight, twaMiddle);
  // Create mesh, attach texture and do cleanup
  Result := TVoxMesh.Create(BannerVertices, [0, 1, 2, 0, 2, 3]);
  Result.LoadTexture(Bitmap);
  Result.Origin := VecA;
  Bitmap.Free();
end;

function MeshNormalSpikes(const Mesh: TVoxMesh; SpikeLength: Double = 0.1): TVoxMesh;
var
  I, J: SizeInt;
begin
  Result := TVoxMesh.Create();
  SetLength(Result.Vertices, Length(Mesh.Vertices) * 2);
  SetLength(Result.Elements, Length(Result.Vertices));
  for I := 0 to Length(Mesh.Vertices) - 1 do begin
    J := I * 2;
    Result.Vertices[J] := Mesh.Vertices[I];
    Result.Vertices[J + 1] := Mesh.Vertices[I];
    Result.Vertices[J + 1].Position += Result.Vertices[J + 1].Normal * SpikeLength;
    Result.Elements[J] := J;
    Result.Elements[J + 1] := J + 1;
  end;
  Result.DrawType := GL_LINES;
end;

{$endif}

{initialization}
{$ifdef INCLUDE_SECTION_INITIALIZATION}

begin
  VOX_FONT_LABEL_DEFAULT := TResourceVectorFont.Create('FONT_MEASUREMENTS');
end;

{$endif}

{finalization}
{$ifdef INCLUDE_SECTION_FINALIZATION}

begin
  VOX_FONT_LABEL_DEFAULT.Free();
end;

{$endif}
