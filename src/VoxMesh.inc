{interface}
{$ifdef INCLUDE_SECTION_INTERFACE}

type
  TBooleanArray = array of Boolean;
  TVertex = packed record
    Position: TVec3;
    Normal: TVec3;
    TexCoord: TVec2;
    Color: TColor4;
    class operator =(const VertA, VertB: TVertex): Boolean;
  end;
  PVertex = ^TVertex;
  TVoxTextureList = specialize TArrayList<TVoxTexture>;
  TVoxMeshMetaType = (mtDefault = 0, mtPassive = 1);
  TVoxMesh = class;
  TVoxMeshSelf = TVoxMesh;
  TVoxMeshList = specialize TArrayList<TVoxMesh>;
  TVoxMeshArray = array of TVoxMesh;
  TVoxMesh = class(TStoredErrorObject)
  private
    // OpenGL objects
    VBO, VAO, EBO: GLuint;
    IsUploadNeeded: Boolean;
    FParent: TVoxMesh;
    FVisible: Boolean;
    function GetVisible(): Boolean;
    procedure SetParent(const NewParent: TVoxMesh);
    function OnChangeChildren(const Sender: TVoxMeshList; EventType: TArrayListEventType;
                              Index: SizeInt; var Value: TVoxMesh; const {%H-}CustomData: Pointer): Boolean;
    procedure RecursiveGetTotalChildren(var Counter: Integer);
    function GetTotalChildren(): Integer;
    procedure RecursiveGetDescendants(const DestinationList: TVoxMeshArray; var Index: Integer);
    function GetDescendants(): TVoxMeshArray;
  public
    // Geometry buffers
    Vertices: array of TVertex;
    Elements: array of GLuint;
    // Mesh attributes
    ID: Integer;
    Data: Pointer;
    Origin: TVec3;
    Material: TVoxPhongMaterial;
    Textures: TVoxTextureList;
    Children: TVoxMeshList;
    MetaType: TVoxMeshMetaType;
    LineWidth: GLfloat;
    // Other properties
    DrawType: GLenum;
    UsageHint: GLenum;
    MayBeFreed: Boolean;
    constructor Create();
    constructor Create(AParent: TVoxMesh);
    constructor Create(const SourceVertices: array of TVertex; const SourceElements: array of GLuint);
    constructor Create(const TrianglePoints: array of TVec3);
    destructor Destroy(); override;
    procedure Free(); reintroduce;
    function IsChildOf(const ParentOrAncestor: TVoxMesh): Boolean;
    // Mesh duplication utilities
    procedure LoadFromBuffers(const SourceVertices: array of TVertex; const SourceElements: array of GLuint);
    function Clone(): TVoxMesh;
    // Transformation functions (move, rotate, scale, etc.)
    function MoveTo(const Position: TVec3; IgnoreChildren: Boolean = False): TVoxMeshSelf;
    function Transform(const Matrix: TMatrix4; IgnoreChildren: Boolean = False): TVoxMeshSelf;
    function Translate(Delta: TVec3; IgnoreChildren: Boolean = False): TVoxMeshSelf;
    function Rotate(const Axis: TVec3; AngleDeg: GLfloat; IgnoreChildren: Boolean = False): TVoxMeshSelf;
    function Scale(Factors: TVec3; IgnoreChildren: Boolean = False): TVoxMeshSelf;
    function Scale(Factor: Double; IgnoreChildren: Boolean = False): TVoxMeshSelf; overload;
    function Mirror(const Axis: TVec3; IgnoreChildren: Boolean = False): TVoxMeshSelf;
    // Non-position mutation functions (normals, colors, textures, etc.)
    function RecalculateNormals(): TVoxMeshSelf;
    function FlipNormals(): TVoxMeshSelf;
    function ShadeSmooth(RespectTexCoords: Boolean = False; const TriangleSmoothness: TBooleanArray = Nil;
                         Epsilon: GLfloat = 1e-10): TVoxMeshSelf;
    function ShadeFlat(): TVoxMeshSelf;
    function Subdivide(): TVoxMeshSelf;
    function Subdivide(Iterations: Integer): TVoxMeshSelf;
    function Merge(const OtherMesh: TVoxMesh; FreeOtherMesh: Boolean = True): TVoxMeshSelf;
    function SetColor(Color: TColor4): TVoxMeshSelf;
    function SetAlpha(Alpha: GLfloat): TVoxMeshSelf;
    function SetAlbedo(Albedo: TColor4): TVoxMeshSelf;
    function LoadTexture(const Image: TBGRABitmap; const Format: TVoxTexForm; Index: Integer = -1): TVoxMeshSelf;
    function LoadTexture(const Image: TBGRABitmap; Index: Integer = -1): TVoxMeshSelf; overload;
    // Various methods
    procedure UploadToGPU();
    procedure Draw(const Shader: TVoxShader; const PrependedTextures: array of TVoxTexture;
                   MeshIndex: Integer = 0);
    function GetBounds(out MinAABB, MaxAABB: TVec3): TVec3;
    function GetBounds(): TVec3; overload;
    function ToString(): String; override;
    property Visible: Boolean read GetVisible write FVisible;
    property Parent: TVoxMesh read FParent write SetParent;
    property TotalChildren: Integer read GetTotalChildren;
    property Descendants: TVoxMeshArray read GetDescendants;
  end;
  TVoxMeshArrayHelper = type helper for TVoxMeshArray
    function IndexOf(const Mesh: TVoxMesh): Integer;
    function MergeAll(): TVoxMesh;
  end;

function Vertex(PX, PY, PZ: GLfloat): TVertex;
function Vertex(Position: TVec3): TVertex;
function Vertex(Position, Normal: TVec3): TVertex;
function Vertex(Position: TVec3; TexCoord: TVec2): TVertex;
function Vertex(Position: TVec3; Color: TColor4): TVertex;
function Vertex(Position, Normal: TVec3; Color: TColor4): TVertex;

{$endif}

{implementation}
{$ifdef INCLUDE_SECTION_IMPLEMENTATION}

type
  TVertexBuffer = specialize TArrayList<TVertex>;
  TBooleanBuffer = specialize TArrayList<Boolean>;

function Vertex(PX, PY, PZ: GLfloat): TVertex;
begin
  Result.Position := Vec3(PX, PY, PZ);
  Result.Normal := Vec3(0.0, 0.0, 0.0);
  Result.TexCoord := Vec2(NaN, NaN);
  Result.Color := COLOR4_WHITE;
end;

function Vertex(Position: TVec3): TVertex;
begin
  Result.Position := Position;
  Result.Normal := Vec3(0.0, 0.0, 0.0);
  Result.TexCoord := Vec2(NaN, NaN);
  Result.Color := COLOR4_WHITE;
end;

function Vertex(Position, Normal: TVec3): TVertex;
begin
  Result.Position := Position;
  Result.Normal := Normal;
  Result.TexCoord := Vec2(NaN, NaN);
  Result.Color := COLOR4_WHITE;
end;

function Vertex(Position: TVec3; TexCoord: TVec2): TVertex;
begin
  Result.Position := Position;
  Result.Normal := Vec3(0.0, 0.0, 0.0);
  Result.TexCoord := TexCoord;
  Result.Color := COLOR4_WHITE;
end;

function Vertex(Position: TVec3; Color: TColor4): TVertex;
begin
  Result.Position := Position;
  Result.Normal := Vec3(0.0, 0.0, 0.0);
  Result.TexCoord := Vec2(NaN, NaN);
  Result.Color := Color;
end;

function Vertex(Position, Normal: TVec3; Color: TColor4): TVertex;
begin
  Result.Position := Position;
  Result.Normal := Normal;
  Result.TexCoord := Vec2(NaN, NaN);
  Result.Color := Color;
end;

procedure Swap(var VecA, VecB: TVec3); inline;
var
  Temp: TVec3;
begin
  Temp := VecB;
  VecB := VecA;
  VecA := Temp;
end;

// #########################################################
// ###                      TVertex                      ###
// #########################################################

class operator TVertex.=(const VertA, VertB: TVertex): Boolean;
begin
  Result := CompareByte(VertA, VertB, SizeOf(VertA)) = 0;
end;

// #########################################################
// ###                     TVoxMesh                      ###
// #########################################################

procedure TVoxMesh.SetParent(const NewParent: TVoxMesh);
begin
  if NewParent <> Nil then begin                            // Adoption
    if NewParent = FParent then Exit();                     // Already adopted by this parent
    if NewParent = Self then Exit();                        // Prevent self-reference
    if NewParent.IsChildOf(Self) then Exit();               // Prevent circular references
    if FParent <> Nil then FParent.Children.Remove(Self);
    FParent := NewParent;
    if not NewParent.Children.Contains(Self) then begin     // Prevent duplicate children
      NewParent.Children.Push(Self);
    end;
  end else if FParent <> Nil then begin                     // Orphaning
    FParent.Children.Remove(Self);                          // Prevent multiple parents
    FParent := Nil;
  end;
end;

function TVoxMesh.OnChangeChildren(const Sender: TVoxMeshList; EventType: TArrayListEventType;
                                   Index: SizeInt; var Value: TVoxMesh; const CustomData: Pointer): Boolean;
begin
  Result := True;
  case EventType of
    alAdd: begin
      Value.Parent := Self;
      if Value.Parent <> Self then Result := False;   // Parent denied adoption
    end;
    alRemove: begin
      Value.Parent := Nil;                            // Parent orphans child
    end;
  end;
  if Parent <> Nil then Parent.Children.OnChange(Sender, alCustom, Index, Value, Value);
end;

procedure TVoxMesh.RecursiveGetTotalChildren(var Counter: Integer);
var
  Child: TVoxMesh;
begin
  Counter += 1;
  for Child in Children do Child.RecursiveGetTotalChildren(Counter);
end;

function TVoxMesh.GetTotalChildren(): Integer;
var
  Counter: Integer = -1;
begin
  RecursiveGetTotalChildren(Counter);
  Result := Counter;
end;

procedure TVoxMesh.RecursiveGetDescendants(const DestinationList: TVoxMeshArray; var Index: Integer);
var
  Child: TVoxMesh;
begin
  if Index >= 0 then DestinationList[Index] := Self;
  Index += 1;
  for Child in Children do Child.RecursiveGetDescendants(DestinationList, Index);
end;

function TVoxMesh.GetDescendants(): TVoxMeshArray;
var
  Index: Integer = -1;
begin
  Result := Default(TVoxMeshArray);
  SetLength(Result, TotalChildren);
  RecursiveGetDescendants(Result, Index);
end;

function TVoxMesh.GetVisible(): Boolean;
var
  Mesh: TVoxMesh;
begin
  Mesh := Self;
  while Mesh.Parent <> Nil do begin
    if not Mesh.FVisible then Exit(False);
    Mesh := Mesh.Parent;
  end;
  Result := FVisible;
end;

constructor TVoxMesh.Create();
const
  SZ_POSITION = SizeOf(Vertices[0].Position);
  SZ_NORMAL = SizeOf(Vertices[0].Normal);
  SZ_TEXCOORD = SizeOf(Vertices[0].TexCoord);
begin
  // Generate vertex buffer, element buffer and vertex array object
  glGenBuffers(1, @VBO);
  glGenBuffers(1, @EBO);
  glGenVertexArrays(1, @VAO);
  // Bind vertex/element buffers to VAO
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

  // Specify vertex format (TVertex)
  // layout(location = 0) in vec3 a_position;
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, SizeOf(Vertices[0]), PGLvoid(0));
  glEnableVertexAttribArray(0);
  // layout(location = 1) in vec3 a_normal;
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, SizeOf(Vertices[0]), PGLvoid(SZ_POSITION));
  glEnableVertexAttribArray(1);
  // layout(location = 2) in vec2 a_texcoord;
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, SizeOf(Vertices[0]), PGLvoid(SZ_POSITION + SZ_NORMAL));
  glEnableVertexAttribArray(2);
  // layout(location = 3) in vec4 a_color;
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, SizeOf(Vertices[0]), PGLvoid(SZ_POSITION + SZ_NORMAL + SZ_TEXCOORD));
  glEnableVertexAttribArray(3);
  // Cleanup
  glBindVertexArray(0);
  IsUploadNeeded := True;

  // Various initializations
  Parent := Nil;
  Visible := True;
  Children := TVoxMeshList.Create();
  Children.OnChange := @OnChangeChildren;
  Material := PHONG_MATERIAL_DEFAULT;
  Textures := TVoxTextureList.Create();
  Origin := Vec3(0.0, 0.0, 0.0);
  LineWidth := 1.0;
  DrawType := GL_TRIANGLES;
  UsageHint := GL_STATIC_DRAW;
  ID := -1;
  MayBeFreed := True;
end;

constructor TVoxMesh.Create(AParent: TVoxMesh);
begin
  Create();
  Parent := AParent;
end;

constructor TVoxMesh.Create(const SourceVertices: array of TVertex; const SourceElements: array of GLuint);
begin
  Create();
  LoadFromBuffers(SourceVertices, SourceElements);
end;

constructor TVoxMesh.Create(const TrianglePoints: array of TVec3);
var
  I: SizeInt;
begin
  Create();
  SetLength(Vertices, Length(TrianglePoints));
  SetLength(Elements, Length(TrianglePoints));
  for I := 0 to Length(TrianglePoints) - 1 do begin
    Vertices[I] := Vertex(TrianglePoints[I]);
    Elements[I] := I;
  end;
  RecalculateNormals();
end;

destructor TVoxMesh.Destroy();
var
  Texture: TVoxTexture;
begin
  glDeleteBuffers(1, @VBO);
  glDeleteBuffers(1, @EBO);
  glDeleteVertexArrays(1, @VAO);
  for Texture in Textures do glDeleteTextures(1, @Texture.Name);
  Textures.Free();
  Children.Free();
  inherited Destroy();
end;

procedure TVoxMesh.Free();
begin
  // Because of GUI multithreading, this mesh might currently be in use by the renderer and may thus not be freed yet
  while not MayBeFreed do Sleep(1);
  Children.OnChange := Nil;
  while Children.Count > 0 do Children.Pop().Free();
  Parent := Nil; // Prevent access-after-free; removes itself from its parent's child list
  inherited Free();
end;

function TVoxMesh.IsChildOf(const ParentOrAncestor: TVoxMesh): Boolean;
begin
  if (FParent = Nil) or (ParentOrAncestor = Nil) then Exit(False); // Orphaned or invalid call
  if FParent <> ParentOrAncestor then Exit(FParent.IsChildOf(ParentOrAncestor)); //Recurse up the family tree
  Result := ParentOrAncestor <> Self; // Implies FParent = ParentOrAncestor
end;

procedure TVoxMesh.LoadFromBuffers(const SourceVertices: array of TVertex; const SourceElements: array of GLuint);
begin
  SetLength(Vertices, Length(SourceVertices));
  Move(SourceVertices[0], Vertices[0], SizeOf(Vertices[0]) * Length(SourceVertices));
  SetLength(Elements, Length(SourceElements));
  Move(SourceElements[0], Elements[0], SizeOf(Elements[0]) * Length(SourceElements));
  IsUploadNeeded := True;
end;

function TVoxMesh.Clone(): TVoxMesh;
begin
  Result := TVoxMesh.Create(Vertices, Elements);
  Result.Parent := Parent;
  Result.Origin := Origin;
  Result.Material := Material;
  Result.Textures.Push(Textures);
  Result.DrawType := DrawType;
  Result.UsageHint := UsageHint;
  Result.ID := ID;
  Result.MayBeFreed := MayBeFreed;
end;

function TVoxMesh.MoveTo(const Position: TVec3; IgnoreChildren: Boolean = False): TVoxMeshSelf;
var
  Adjustment: TVec3;
  Child: TVoxMesh;
  I: SizeInt;
begin
  Adjustment := Position - Origin;
  if not IgnoreChildren then for Child in Children do begin
    Child.MoveTo(Child.Origin + Adjustment, False);
  end;
  for I := 0 to Length(Vertices) - 1 do with Vertices[I] do begin
    Position += Adjustment;
  end;
  Origin := Position;
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.Transform(const Matrix: TMatrix4; IgnoreChildren: Boolean = False): TVoxMeshSelf;
var
  NormalMatrix: TMatrix4; // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
  Child: TVoxMesh;
  I: SizeInt;
begin
  if not IgnoreChildren then for Child in Children do begin
    Child.Origin := Matrix * (Child.Origin - Origin) + Origin;
    Swap(Child.Origin, Origin); // Temporarily set child position to parent position
    Child.Transform(Matrix, False);
    Swap(Child.Origin, Origin);
  end;
  NormalMatrix := Matrix;
  NormalMatrix[3, 0] := 0.0; // Since the "offset" part of the normal matrix is zero...
  NormalMatrix[3, 1] := 0.0; // ...and its last row is ignored anyways in the end (no W component)...
  NormalMatrix[3, 2] := 0.0; // ...it can be treated as a 3x3 matrix
  NormalMatrix := NormalMatrix.Inverse3x3().Transpose();
  for I := 0 to Length(Vertices) - 1 do with Vertices[I] do begin
    Position := Matrix * (Position - Origin) + Origin;
    Normal := (NormalMatrix * Normal).Normal();
  end;
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.Translate(Delta: TVec3; IgnoreChildren: Boolean = False): TVoxMeshSelf;
begin
  Result := Transform(TranslationMatrix(Delta), IgnoreChildren);
end;

function TVoxMesh.Rotate(const Axis: TVec3; AngleDeg: GLfloat; IgnoreChildren: Boolean = False): TVoxMeshSelf;
begin
  Result := Transform(RotationMatrix(Axis, DegToRad(AngleDeg)), IgnoreChildren);
end;

function TVoxMesh.Scale(Factors: TVec3; IgnoreChildren: Boolean = False): TVoxMeshSelf;
begin
  Result := Transform(ScaleMatrix(Factors), IgnoreChildren);
end;

function TVoxMesh.Scale(Factor: Double; IgnoreChildren: Boolean = False): TVoxMeshSelf;
begin
  Result := Transform(ScaleMatrix(Vec3(Factor, Factor, Factor)), IgnoreChildren);
end;

function TVoxMesh.Mirror(const Axis: TVec3; IgnoreChildren: Boolean = False): TVoxMeshSelf;
var
  Temp: GLuint;
  I: SizeInt;
begin
  Result := Transform(ReflectionMatrix(Axis), IgnoreChildren);
  for I := 0 to Length(Elements) div 3 - 1 do begin // Flip triangle winding direction
    Temp := Elements[I * 3 + 2];
    Elements[I * 3 + 2] := Elements[I * 3 + 0];
    Elements[I * 3 + 0] := Temp;
  end;
end;

function TVoxMesh.RecalculateNormals(): TVoxMeshSelf;
var
  TriNormal: TVec3;
  VxA, VxB, VxC: PVertex;
  I: SizeInt;
  function TriangleNormal(const VecA, VecB, VecC: TVec3): TVec3;
  begin
    Result := (VecC - VecB).Cross(VecA - VecB).Normal();
  end;
begin
  for I := 0 to Length(Vertices) - 1 do with Vertices[I] do begin
    Normal := Vec3(0.0, 0.0, 0.0); // Reset normals to (0, 0, 0)
  end;
  for I := 0 to Length(Elements) div 3 - 1 do begin // Sum calculated normals
    VxA := @Vertices[Elements[I * 3 + 0]]; VxB := @Vertices[Elements[I * 3 + 1]]; VxC := @Vertices[Elements[I * 3 + 2]];
    TriNormal := TriangleNormal(VxA^.Position, VxB^.Position, VxC^.Position);
    VxA^.Normal += TriNormal;
    VxB^.Normal += TriNormal;
    VxC^.Normal += TriNormal;
  end;
  for I := 0 to Length(Vertices) - 1 do with Vertices[I] do begin // Calculate average normals
    Normal := Normal.Normal();
  end;
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.FlipNormals(): TVoxMeshSelf;
var
  I: SizeInt;
begin
  for I := 0 to Length(Vertices) - 1 do with Vertices[I] do begin
    Normal := -Normal;
  end;
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.ShadeSmooth(RespectTexCoords: Boolean = False; const TriangleSmoothness: TBooleanArray = Nil;
                              Epsilon: GLfloat = 1e-10): TVoxMeshSelf;
var
  NewVertices: TVertexBuffer;
  NewSmooths: TBooleanBuffer;
  I: SizeInt;
  function ClosestNewElement(OldElement: GLuint): GLuint; inline;
  var
    OldVertex: TVertex;
    CanBeSmooth: Boolean;
    J: Integer;
  begin
    OldVertex := Vertices[OldElement];
    CanBeSmooth := (TriangleSmoothness = Nil) or TriangleSmoothness[I div 3];
    if CanBeSmooth then begin
      for J := NewVertices.Count - 1 downto 0 do begin
        if not NewSmooths[J] then continue;
        if (NewVertices[J].Position - OldVertex.Position).LengthSqr() > Epsilon then continue;
        if (NewVertices[J].Normal - OldVertex.Normal).LengthSqr() > Epsilon then continue;
        if RespectTexCoords and ((NewVertices[J].TexCoord - OldVertex.TexCoord).LengthSqr() > Epsilon) then continue;
        if TVec4(NewVertices[J].Color - OldVertex.Color).LengthSqr() > Epsilon then continue;
        Exit(J); // Vertex is already known
      end;
    end;
    Result := NewVertices.Count;
    NewVertices.Push(OldVertex); // Vertex is new
    NewSmooths.Push(CanBeSmooth);
  end;
begin
  NewVertices := TVertexBuffer.Create(SizeOf(Vertices[0]) * Length(Vertices));
  NewSmooths := TBooleanBuffer.Create(SizeOf(Boolean) * Length(Vertices));
  for I := 0 to Length(Elements) - 1 do begin
    Elements[I] := ClosestNewElement(Elements[I]);
  end;
  Vertices := NewVertices.Items;
  NewVertices.Free();
  NewSmooths.Free();
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.ShadeFlat(): TVoxMeshSelf;
var
  NewVertices: array of TVertex;
  I: SizeInt;
begin
  SetLength(NewVertices{%H-}, Length(Elements));
  for I := 0 to Length(NewVertices) - 1 do begin
    NewVertices[I] := Vertices[Elements[I]];
    Elements[I] := I;
  end;
  SetLength(Vertices, Length(NewVertices));
  Move(NewVertices[0], Vertices[0], SizeOf(Vertices[0]) * Length(Vertices));
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.Subdivide(): TVoxMeshSelf;
var
  NewVertices: array of TVertex;
  NewElements: array of GLuint;
  OldTriangleCount: Integer;
  OVA, OVB, OVC: TVertex; // Old vertices
  NVAB, NVBC, NVCA: TVertex; // New vertices
  I, J, K, L: Integer;
  procedure AvgVertex(const VertA, VertB: PVertex; out Result: TVertex);
  begin
    Result.Position := (VertA^.Position + VertB^.Position) * 0.5;
    Result.Normal := ((VertA^.Normal + VertB^.Normal) * 0.5).Normal();
    Result.TexCoord := (VertA^.TexCoord + VertB^.TexCoord) * 0.5;
    Result.Color := (VertA^.Color + VertB^.Color) * 0.5;
  end;
begin
  OldTriangleCount := Length(Elements) div 3;
  SetLength(NewVertices{%H-}, Length(Vertices) * 4); // One source triangle maps to 4 subdivided triangles
  SetLength(NewElements{%H-}, Length(NewVertices));
  for I := 0 to OldTriangleCount - 1 do begin
    J := I * 3; // Old triangle vertex/element index
    K := I * 12; // New triangle vertex/element index
    OVA := Vertices[Elements[J]]; OVB := Vertices[Elements[J + 1]]; OVC := Vertices[Elements[J + 2]];
    AvgVertex(@OVA, @OVB, NVAB); AvgVertex(@OVB, @OVC, NVBC); AvgVertex(@OVA, @OVC, NVCA);
    NewVertices[K + $0] := NVAB; NewVertices[K + $1] := NVBC; NewVertices[K + $2] := NVCA; // Triangle 1
    NewVertices[K + $3] := OVA;  NewVertices[K + $4] := NVAB; NewVertices[K + $5] := NVCA; // Triangle 2
    NewVertices[K + $6] := OVB;  NewVertices[K + $7] := NVBC; NewVertices[K + $8] := NVAB; // Triangle 3
    NewVertices[K + $9] := OVC;  NewVertices[K + $A] := NVCA; NewVertices[K + $B] := NVBC; // Triangle 4
    for L := K to K + 11 do NewElements[L] := L;
  end;
  LoadFromBuffers(NewVertices, NewElements);
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.Subdivide(Iterations: Integer): TVoxMeshSelf;
var
  I: SizeInt;
begin
  for I := 0 to Iterations - 1 do Subdivide();
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.Merge(const OtherMesh: TVoxMesh; FreeOtherMesh: Boolean = True): TVoxMeshSelf;
var
  OldVertexCount, OldElementCount: SizeInt;
  I: SizeInt;
begin
  OldVertexCount := Length(Vertices);
  OldElementCount := Length(Elements);
  SetLength(Vertices, OldVertexCount + Length(OtherMesh.Vertices));
  SetLength(Elements, OldElementCount + Length(OtherMesh.Elements));
  Move(OtherMesh.Vertices[0], Vertices[OldVertexCount], SizeOf(Vertices[0]) * Length(OtherMesh.Vertices));
  for I := 0 to Length(OtherMesh.Elements) - 1 do begin
    Elements[I + OldElementCount] := OtherMesh.Elements[I] + GLuint(OldVertexCount);
  end;
  if FreeOtherMesh then begin
    Children.Push(OtherMesh.Children);
    OtherMesh.Free();
  end;
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.SetColor(Color: TColor4): TVoxMeshSelf;
var
  I: SizeInt;
begin
  for I := 0 to Length(Vertices) - 1 do Vertices[I].Color := Color;
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.SetAlpha(Alpha: GLfloat): TVoxMeshSelf;
var
  I: SizeInt;
begin
  for I := 0 to Length(Vertices) - 1 do Vertices[I].Color.A := Alpha;
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.SetAlbedo(Albedo: TColor4): TVoxMeshSelf;
var
  I: SizeInt;
begin
  for I := 0 to Length(Vertices) - 1 do Vertices[I].Color := Color4(Albedo, Vertices[I].Color.A);
  IsUploadNeeded := True;
  Result := Self;
end;

function TVoxMesh.LoadTexture(const Image: TBGRABitmap; const Format: TVoxTexForm; Index: Integer = -1): TVoxMeshSelf;
var
  NewTexture: TVoxTexture;
begin
  // Generate texture
  NewTexture.Format := Format;
  NewTexture.UploadToGPU(Image);
  // Add or replace object texture
  if (Index < 0) or (Index >= Textures.Count) then begin
    Textures.Push(NewTexture);
  end else Textures.P[Index]^ := NewTexture;
  Result := Self;
end;

function TVoxMesh.LoadTexture(const Image: TBGRABitmap; Index: Integer = -1): TVoxMeshSelf;
var
  Format: TVoxTexForm;
begin
  Format := TEX_FORM_COLOR;
  Format.InternalFormat := GL_SRGB_ALPHA;
  Result := LoadTexture(Image, Format, Index);
end;

procedure TVoxMesh.UploadToGPU();
begin
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, SizeOf(Vertices[0]) * Length(Vertices), @Vertices[0], UsageHint);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, SizeOf(Elements[0]) * Length(Elements), @Elements[0], UsageHint);
end;

procedure TVoxMesh.Draw(const Shader: TVoxShader; const PrependedTextures: array of TVoxTexture;
                        MeshIndex: Integer = 0);
var
  PreTexCount: Integer;
  I: Integer;
begin
  if not Visible then Exit();
  // Bind textures
  PreTexCount := Length(PrependedTextures);
  for I := 0 to PreTexCount - 1 do begin
    glActiveTexture(GL_TEXTURE0 + I);
    glBindTexture(PrependedTextures[I].Format.Target, PrependedTextures[I].Name);
    glUniform1i(Shader[Format('u_texture%d', [I])], I);
  end;
  for I := PreTexCount to PreTexCount + Textures.Count - 1 do begin
    glActiveTexture(GL_TEXTURE0 + I);
    glBindTexture(Textures[I - PreTexCount].Format.Target, Textures[I - PreTexCount].Name);
    glUniform1i(Shader[Format('u_texture%d', [I])], I);
  end;
  // Object uniforms
  glUniform1i(Shader['u_mesh_index'], MeshIndex);
  glUniform1i(Shader['u_mesh_texcount'], Textures.Count);
  // Phong material
  with Material do begin
    glUniform3f(Shader['u_phong_material.ambient'], Ambient.R, Ambient.G, Ambient.B);
    glUniform3f(Shader['u_phong_material.diffuse'], Diffuse.R, Diffuse.G, Diffuse.B);
    glUniform3f(Shader['u_phong_material.specular'], Specular.R, Specular.G, Specular.B);
    glUniform1f(Shader['u_phong_material.shininess'], Shininess);
    glUniform1f(Shader['u_phong_material.alpha'], Alpha);
  end;
  // Draw using VAO
  If IsUploadNeeded then begin
    UploadToGPU();
    IsUploadNeeded := False;
  end;
  glLineWidth(LineWidth);
  glBindVertexArray(VAO);
  glDrawElements(DrawType, Length(Elements), GL_UNSIGNED_INT, PGLvoid(0));
  glBindVertexArray(0);
end;

function TVoxMesh.GetBounds(out MinAABB, MaxAABB: TVec3): TVec3;
var
  I: SizeInt;
begin
  if Length(Vertices) = 0 then begin
    MinAABB := Vec3(NaN, NaN, NaN);
    MaxAABB := Vec3(NaN, NaN, NaN);
    Exit(Vec3(0.0, 0.0, 0.0));
  end;
  MinAABB := Vec3(Infinity, Infinity, Infinity);
  MaxAABB := Vec3(-Infinity, -Infinity, -Infinity);
  for I := 0 to Length(Vertices) - 1 do with Vertices[I].Position do begin
    MinAABB := Vec3(Min(X, MinAABB.X), Min(Y, MinAABB.Y), Min(Z, MinAABB.Z));
    MaxAABB := Vec3(Max(X, MaxAABB.X), Max(Y, MaxAABB.Y), Max(Z, MaxAABB.Z));
  end;
  Result := MaxAABB - MinAABB;
end;

function TVoxMesh.GetBounds(): TVec3;
var
  MinAABB, MaxAABB: TVec3;
begin
  Result := GetBounds(MinAABB, MaxAABB);
end;

function TVoxMesh.ToString(): String;
var
  MetaTypeStr: String;
  I: Integer;
  function FormatPointer(Ptr: Pointer): String;
  begin
    Result := IfThen(Ptr = Nil, 'nil', AnsiLowerCase(Format('0x%p', [Ptr])));
  end;
begin
  WriteStr(MetaTypeStr, MetaType);
  Result := Format('TVoxMesh [%s] {', [FormatPointer(Pointer(Self))]) + LineEnding;
  Result += Format('  ID: %d,', [ID]) + LineEnding;
  Result += Format('  Origin: %s,', [Origin.ToString()]) + LineEnding;
  Result += Format('  Visible: %s,', [IfThen(FVisible, 'true', 'false')]) + LineEnding;
  Result += Format('  MetaType: %s,', [MetaTypeStr]) + LineEnding;
  Result += Format('  Textures: <%u>,', [Textures.Count]) + LineEnding;
  Result += Format('  Material: %s,', [Material.ToString().Replace(LineEnding, LineEnding + '  ')]) + LineEnding;
  Result += Format('  Vertices: <%u>, Elements: <%u>,', [Length(Vertices), Length(Elements)]) + LineEnding;
  Result += Format('  VBO: [%u],', [VBO]) + LineEnding;
  Result += Format('  EBO: [%u],', [EBO]) + LineEnding;
  Result += Format('  Data: %s,', [FormatPointer(Data)]) + LineEnding;
  Result += Format('  DrawType: [%u],', [DrawType]) + LineEnding;
  Result += Format('  Children: TVoxMeshList[%d] {', [Children.Count]) + LineEnding;
  for I := 0 to Children.Count - 1 do begin
    Result += '    ' + Children[I].ToString().Replace(LineEnding, LineEnding + '    ');
    if I < (Children.Count - 1) then Result += ',';
    Result += LineEnding;
  end;
  Result += Format('  }', []) + LineEnding;
  Result += Format('}', []);
end;

// #########################################################
// ###                TVoxMeshArrayHelper                ###
// #########################################################

function TVoxMeshArrayHelper.IndexOf(const Mesh: TVoxMesh): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to Length(Self) - 1 do begin
    if Self[I] = Mesh then Exit(I);
  end;
end;

function TVoxMeshArrayHelper.MergeAll(): TVoxMesh;
var
  I: Integer;
begin
  for I := 1 to Length(Self) - 1 do Self[0].Merge(Self[I]);
  if Length(Self) > 0 then begin
    SetLength(Self, 1);
    Result := Self[0];
  end else Result := Nil;
end;

{$endif}
