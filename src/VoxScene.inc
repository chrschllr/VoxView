{interface}
{$ifdef INCLUDE_SECTION_INTERFACE}

type
  TVoxProjection = (prPerspective, prOrthographic);
  TVoxCamera = record
  public
    Position, Direction: TVec3;
    VisualWorldUp: TVec3;
    AspectRatio, FOV, OrthoSize: GLfloat;
    NearPlane, FarPlane: GLfloat;
    Projection: TVoxProjection;
    constructor Create(Aspect: GLfloat);
    function UpVector(): TVec3;
    function RightVector(): TVec3;
    function ModelMatrix(): TMatrix4;
    function ViewMatrix(): TMatrix4;
    function ProjectionMatrix(): TMatrix4;
    function Project(const Point: TVec3; Invert: Boolean = False): TVec3;
    procedure LookAt(Target: TVec3);
    function ToString(): String;
  end;
  TVoxUniformSetter = procedure(const Shader: TVoxShader) of object;
  TVoxRendererList = specialize TArrayList<TVoxRenderer>;
  TVoxShaderList = specialize TArrayList<TVoxShader>;
  TVoxScene = class(TStoredErrorObject)
  const
    DEFAULT_GAMMA = 2.22;
    DEFAULT_CLEAR_COLOR: TColor4 = (R: 0.129; G: 0.157; B: 0.196; A: 1.0);
    DEFAULT_GRID_COLOR: TColor4 = (R: 0.500; G: 0.500; B: 0.500; A: 0.1);
    DEFAULT_GRID_RADIUS = 1000;
  private
    FWidth, FHeight: Integer;
    FSamples: Integer;
    FMeshes: TVoxMeshList;
    FActiveRenderer: TVoxRenderer;
    MeshHierarchyChanged: Boolean;
    function GetMeshes(): TVoxMeshList;
    function OnChangeMeshes(const {%H-}Sender: TVoxMeshList; EventType: TArrayListEventType;
                       {%H-}Index: SizeInt; var Value: TVoxMesh; const {%H-}CustomData: Pointer): Boolean;
    function OnChangeMeshHierarchy(const {%H-}Sender: TVoxMeshList; {%H-}EventType: TArrayListEventType;
                              {%H-}Index: SizeInt; var {%H-}Value: TVoxMesh; const {%H-}CustomData: Pointer): Boolean;
  protected // Should only be visible within this package (could also be private as per FPC, but I don't like that)
    RootMesh: TVoxMesh;
    GridShader: TVoxShader;
  public
    StartTimestamp: TDateTime;
    Camera: TVoxCamera;
    Gamma: GLfloat;
    ClearColor: TColor4;
    Lamps: TVoxPhongLampList;
    Renderers: TVoxRendererList;
    MetaRenderer: TVoxRendererMeta;
    ScreenFramebuffer: TVoxFramebuffer;
    ScreenQuad: TVoxMesh;
    PostEffects: TVoxShaderList; // Standard stack: Gamma correction, [object outline], FXAA
    constructor Create(AWidth, AHeight: Integer; ASamples: Integer = 1);
    constructor Create(AWidth, AHeight: Integer; ASamples: Integer; const ScreenVertSource, ScreenFragSource,
                       ModelVertSource, ModelFragSource: String);
    destructor Destroy(); override;
    procedure Resize(NewWidth, NewHeight: Integer);
    procedure SetActiveRenderer(const Renderer: TVoxRenderer);
    procedure SetGrid(Radius: Cardinal; const Color: TColor4);
    procedure Draw(const CustomUniforms: TVoxUniformSetter = Nil);
    function ExportOBJ(const DrawTypeBlacklist: TGLenumArray = Nil; InvertList: Boolean = False): String;
    function ToString(): String; override;
    property Width: Integer read FWidth;
    property Height: Integer read FHeight;
    property Samples: Integer read FSamples;
    property Meshes: TVoxMeshList read GetMeshes;
    property ActiveRenderer: TVoxRenderer read FActiveRenderer write SetActiveRenderer;
  end;

{$endif}

{implementation}
{$ifdef INCLUDE_SECTION_IMPLEMENTATION}

// #########################################################
// ###                    TVoxCamera                     ###
// #########################################################

constructor TVoxCamera.Create(Aspect: GLfloat);
begin
  // Constant used for generating RightVector and UpVector
  VisualWorldUp := Vec3(0.0, 0.0, 1.0);
  // Location, rotation
  Position := Vec3(0.0, -3.0, 0.0);
  Direction := Vec3(0.0, 1.0, 0.0);
  // Projection
  AspectRatio := Aspect;
  FOV := DegToRad(50.0);
  OrthoSize := 2.0;
  Projection := prPerspective;
  // Clipping
  NearPlane := 0.015625;
  FarPlane := 512.0;
end;

function TVoxCamera.UpVector(): TVec3;
begin
  Result := RightVector().Cross(Direction).Normal();
end;

function TVoxCamera.RightVector(): TVec3;
begin
  Result := Direction.Cross(VisualWorldUp).Normal();
end;

function TVoxCamera.ModelMatrix(): TMatrix4;
begin
  Result := Matrix4([
    1, 0, 0, 0,
    0, 0, 1, 0,
    0, 1, 0, 0,
    0, 0, 0, 1
  ]);
end;

function TVoxCamera.ViewMatrix(): TMatrix4;
var
  MatRot, MatTrans, MatModel: TMatrix4;
  Right, Up: TVec3;
begin
  MatModel := ModelMatrix(); // Camera position & direction are both in local space -> transform them first
  Right := RightVector();
  Up := UpVector();
  MatRot := Matrix4([
    Right.X,      Right.Y,      Right.Z,      0.0,  // MatModel as the second factor swaps columns
    Up.X,         Up.Y,         Up.Z,         0.0,  // instead of rows, which is useful on this pseudo-
    Direction.X,  Direction.Y,  Direction.Z, 0.0,   // transposed matrix (because it uses row vectors instead of
    0.0,          0.0,          0.0,          1.0   // the usual column vectors)
  ]) * MatModel;
  MatTrans := TranslationMatrix(MatModel * -Position);
  Result := MatRot * MatTrans;
end;

function TVoxCamera.ProjectionMatrix(): TMatrix4;
var
  FP, NP, I_FP_NP, AR: GLfloat;
  CotF2, S2: GLfloat;
begin
  Result := Default(TMatrix4);
  FP := FarPlane;
  NP := NearPlane;
  I_FP_NP := 1.0 / (FP - NP);
  AR := AspectRatio;
  case Projection of
    prPerspective: begin
      CotF2 := Cot(FOV / 2.0);
      Result := Matrix4([
        CotF2 / AR,   0.0,      0.0,                    0.0,
        0.0,          CotF2,    0.0,                    0.0,
        0.0,          0.0,      (NP + FP) * I_FP_NP,   -2.0 * NP * FP * I_FP_NP,
        0.0,          0.0,      1.0,                    0.0
      ]);
    end;
    prOrthographic: begin
      S2 := 2.0 / OrthoSize;
      Result := Matrix4([
        S2 / AR,  0.0,   0.0,        0.0,
        0.0,      S2,    0.0,        0.0,
        0.0,      0.0,   I_FP_NP,   -I_FP_NP * NP,
        0.0,      0.0,   0.0,        1.0
      ]);
    end;
  end;
end;

function TVoxCamera.Project(const Point: TVec3; Invert: Boolean = False): TVec3;
var
  MatrixStack: TMatrix4;
  HomogenousResult: TVec4;
begin
  MatrixStack := TransformationMatrix([
    ModelMatrix(),
    ViewMatrix(),
    ProjectionMatrix()
  ]);
  if Invert then MatrixStack := MatrixStack.Inverse();
  HomogenousResult := MatrixStack * Vec4(Point, 1.0);
  Result := Vec3(HomogenousResult) / HomogenousResult.W;
end;

procedure TVoxCamera.LookAt(Target: TVec3);
begin
  Direction := (Target - Position).Normal();
end;

function TVoxCamera.ToString(): String;
var
  ProjectionStr: String;
begin
  WriteStr(ProjectionStr, Projection);
  Result := Format('TVoxCamera {', []) + LineEnding;
  Result += Format('  Position: %s,', [Position.ToString()]) + LineEnding;
  Result += Format('  Direction: %s,', [Direction.ToString()]) + LineEnding;
  Result += Format('  AspectRatio: %.3f,', [AspectRatio], VOX_FORMAT) + LineEnding;
  Result += Format('  FOV: %.2f °,', [FOV / Pi() * 180], VOX_FORMAT) + LineEnding;
  Result += Format('  OrthoSize: %.3f,', [OrthoSize], VOX_FORMAT) + LineEnding;
  Result += Format('  NearPlane: %.4e,', [NearPlane], VOX_FORMAT) + LineEnding;
  Result += Format('  FarPlane: %.4e,', [FarPlane], VOX_FORMAT) + LineEnding;
  Result += Format('  Projection: %s', [ProjectionStr]) + LineEnding;
  Result += Format('}', []);
end;

// #########################################################
// ###                    TVoxScene                      ###
// #########################################################

function TVoxScene.GetMeshes(): TVoxMeshList;
begin
  if MeshHierarchyChanged then begin
    MeshHierarchyChanged := False;
    FMeshes.EventsEnabled := False;
    FMeshes.Clear();
    FMeshes.Push(RootMesh.Descendants);
    FMeshes.EventsEnabled := True;
  end;
  Result := FMeshes;
end;

function TVoxScene.OnChangeMeshes(const Sender: TVoxMeshList; EventType: TArrayListEventType;
                                  Index: SizeInt; var Value: TVoxMesh; const CustomData: Pointer): Boolean;
begin
  Result := False; // FMeshes is generated dynamically, so any direct modifications to it can be ignored
  case EventType of
    alAdd: Value.Parent := RootMesh;
    alRemove: Value.Parent := Nil;
    alClear: while RootMesh.Children.Count > 0 do RootMesh.Children.Pop().Free();
  end;
end;

function TVoxScene.OnChangeMeshHierarchy(const Sender: TVoxMeshList; EventType: TArrayListEventType;
                                         Index: SizeInt; var Value: TVoxMesh; const CustomData: Pointer): Boolean;
begin
  Result := True; // This handler is only used to catch events that have been propagated up the family tree...
  MeshHierarchyChanged := True; // ...in order to trigger the invalidation of the (flattened) mesh family tree cache
end;

constructor TVoxScene.Create(AWidth, AHeight: Integer; ASamples: Integer = 1);
begin
  Create(AWidth, AHeight, ASamples, SHADER_VERT_SCREEN, SHADER_FRAG_SCREEN, SHADER_VERT_MODEL, SHADER_FRAG_DEFAULT);
  PostEffects.Push(TVoxShader.Create(SHADER_VERT_SCREEN, SHADER_FRAG_FXAA));
  if not PostEffects.Last^.IsValid then Error := PostEffects.Last^.Error;
end;

constructor TVoxScene.Create(AWidth, AHeight: Integer; ASamples: Integer;
                             const ScreenVertSource, ScreenFragSource, ModelVertSource, ModelFragSource: String);
const
  SCREEN_QUAD_TRIANGLES: array of TVec3 = ( // Normalized device coordinates
    (X: -1; Y: -1; Z: 0), (X: 1; Y: -1; Z: 0), (X: -1; Y: 1; Z: 0),
    (X: 1; Y: 1; Z: 0), (X: -1; Y: 1; Z: 0), (X: 1; Y: -1; Z: 0)
  );
begin
  FWidth := AWidth;
  FHeight := AHeight;
  FSamples := ASamples;
  // Final composite framebuffer and screen-filling quad
  ScreenFramebuffer := TVoxFramebuffer.Create(
    Width, Height, [
      AttachmentInfo(GL_DEPTH_ATTACHMENT, TEX_FORM_DEPTH),
      AttachmentInfo(GL_COLOR_ATTACHMENT0, TEX_FORM_COLOR), // Color render target and post-processing color 0
      AttachmentInfo(GL_COLOR_ATTACHMENT1, TEX_FORM_COLOR)  // Post-processing color 1
    ]
  );
  ScreenQuad := TVoxMesh.Create(SCREEN_QUAD_TRIANGLES);
  PostEffects := TVoxShaderList.Create();
  PostEffects.Push(TVoxShader.Create(ScreenVertSource, ScreenFragSource));
  if not PostEffects[0].IsValid then Error := PostEffects[0].Error;
  // Default rendering engine
  Renderers := TVoxRendererList.Create();
  ActiveRenderer := TVoxRendererDefault.Create(Self, ModelVertSource, ModelFragSource);
  MetaRenderer := TVoxRendererMeta.Create(Self);
  ScreenQuad.Textures.Push([ // Use composite framebuffer as texture for screen-filling quad
    ScreenFramebuffer.ColorAttachments[0],
    ScreenFramebuffer.DepthAttachment.Texture,
    MetaRenderer.IndexAttachment,
    MetaRenderer.PositionAttachment
  ]);
  // Default lighting
  Lamps := TVoxPhongLampList.Create();
  Lamps.Push([
    PHONG_LAMP_4P_WHITE_DEFAULT,
    PHONG_LAMP_4P_ORANGE_DEFAULT,
    PHONG_LAMP_4P_BLUE_DEFAULT,
    PHONG_LAMP_4P_RED_DEFAULT
  ]);
  GridShader := TVoxShader.Create(SHADER_VERT_MODEL, SHADER_FRAG_GRID); // Background grid
  // Mesh tree (RootMesh should not be directly modified; its geometry is only used for drawing i.e. a world grid ^)
  RootMesh := TVoxMesh.Create();
  RootMesh.Children.OnChange := @OnChangeMeshHierarchy;
  RootMesh.MetaType := mtPassive;
  FMeshes := TVoxMeshList.Create();
  FMeshes.OnChange := @OnChangeMeshes;
  Camera := TVoxCamera.Create(Width / Height);
  SetGrid(DEFAULT_GRID_RADIUS, DEFAULT_GRID_COLOR);
  ClearColor := DEFAULT_CLEAR_COLOR;
  Gamma := DEFAULT_GAMMA;
  StartTimestamp := Now();
end;

destructor TVoxScene.Destroy();
begin
  FMeshes.Free();
  RootMesh.Free();
  Lamps.Free();
  Renderers.FreeItems(True);
  MetaRenderer.Free();
  ScreenFramebuffer.Free();
  ScreenQuad.Free();
  PostEffects.FreeItems(True);
  GridShader.Free();
  inherited Destroy();
end;

procedure TVoxScene.Resize(NewWidth, NewHeight: Integer);
begin
  FWidth := NewWidth;
  FHeight := NewHeight;
  Camera.AspectRatio := Width / Height;
  ActiveRenderer.Resize(Width, Height);
  MetaRenderer.Resize(Width, Height);
  ScreenFramebuffer.Resize(Width, Height);
end;

procedure TVoxScene.SetActiveRenderer(const Renderer: TVoxRenderer);
begin
  if not Renderers.Contains(Renderer) then Renderers.Push(Renderer);
  FActiveRenderer := Renderer;
  ActiveRenderer.Resize(Width, Height);
end;

procedure TVoxScene.SetGrid(Radius: Cardinal; const Color: TColor4);
const
  COLOR_AXIS_X: TColor4 = (R: 0.647; G: 0.188; B: 0.188; A: 1.000);
  COLOR_AXIS_Y: TColor4 = (R: 0.459; G: 0.655; B: 0.263; A: 1.000);
  AXIS_EPSILON = 0.0; // TODO: Properly draw axis on top of grid lines
var
  BaseVert: PVertex;
  Dist: GLfloat;
  I: Integer;
begin
  with RootMesh do if Radius > 0 then begin
    DrawType := GL_LINES;
    SetLength(Vertices, 4 + Radius * 16);
    SetLength(Elements, Length(Vertices));
    Vertices[0].Position := Vec3( Radius, 0.0,    0.0);
    Vertices[1].Position := Vec3(-Radius, 0.0,    0.0);
    Vertices[2].Position := Vec3(0.0,     Radius, 0.0);
    Vertices[3].Position := Vec3(0.0,    -Radius, 0.0);
    Vertices[0].Color := COLOR_AXIS_X;
    Vertices[1].Color := COLOR_AXIS_X;
    Vertices[2].Color := COLOR_AXIS_Y;
    Vertices[3].Color := COLOR_AXIS_Y;
    BaseVert := @Vertices[4];
    for I := 0 to Integer(Radius) - 1 do begin
      Dist := I + 1;
      BaseVert[I * 16 + $0].Position := Vec3( Dist,           AXIS_EPSILON, 0.0);
      BaseVert[I * 16 + $1].Position := Vec3( Dist,           Radius,       0.0);
      BaseVert[I * 16 + $2].Position := Vec3( Dist,          -AXIS_EPSILON, 0.0);
      BaseVert[I * 16 + $3].Position := Vec3( Dist,          -Radius,       0.0);
      BaseVert[I * 16 + $4].Position := Vec3(-Dist,           AXIS_EPSILON, 0.0);
      BaseVert[I * 16 + $5].Position := Vec3(-Dist,           Radius,       0.0);
      BaseVert[I * 16 + $6].Position := Vec3(-Dist,          -AXIS_EPSILON, 0.0);
      BaseVert[I * 16 + $7].Position := Vec3(-Dist,          -Radius,       0.0);
      BaseVert[I * 16 + $8].Position := Vec3( AXIS_EPSILON,   Dist,         0.0);
      BaseVert[I * 16 + $9].Position := Vec3( Radius,         Dist,         0.0);
      BaseVert[I * 16 + $A].Position := Vec3(-AXIS_EPSILON,   Dist,         0.0);
      BaseVert[I * 16 + $B].Position := Vec3(-Radius,         Dist,         0.0);
      BaseVert[I * 16 + $C].Position := Vec3( AXIS_EPSILON,  -Dist,         0.0);
      BaseVert[I * 16 + $D].Position := Vec3( Radius,        -Dist,         0.0);
      BaseVert[I * 16 + $E].Position := Vec3(-AXIS_EPSILON,  -Dist,         0.0);
      BaseVert[I * 16 + $F].Position := Vec3(-Radius,        -Dist,         0.0);
    end;
    for I := 4 to Length(Vertices) - 1 do Vertices[I].Color := Color;
    for I := 0 to Length(Vertices) - 1 do Vertices[I].Normal := Vec3(0.0, 0.0, 1.0);
    for I := 0 to Length(Elements) - 1 do Elements[I] := I;
  end else begin
    SetLength(Vertices, 0);
    SetLength(Elements, 0);
  end;
  RootMesh.IsUploadNeeded := True;
end;

procedure TVoxScene.Draw(const CustomUniforms: TVoxUniformSetter = Nil);
var
  ReadIndex, WriteIndex: Integer;
  I: Integer;
begin
  ActiveRenderer.Draw(ScreenFramebuffer); // Draw to screen framebuffer
  // Post-processing shader stack (executed from low to high)
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  for I := 0 to PostEffects.Count - 1 do begin
    ReadIndex := I mod 2;
    WriteIndex := 1 - ReadIndex;
    ScreenQuad.Textures[0] := ScreenFramebuffer.ColorAttachments[ReadIndex];
    if I = (PostEffects.Count - 1) then begin // Final pass -> draw to screen
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glDrawBuffer(GL_BACK);
      glViewport(0, 0, Width, Height);
      glClearColor(0.0, 0.0, 1.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT);
      glDisable(GL_BLEND);
    end else ScreenFramebuffer.Bind([WriteIndex]);
    // Draw to screen via fullscreen quad (texture0 = color[switched], texture1 = depth, texture2... = meta...)
    glUseProgram(PostEffects[I].ShaderProgram);
    glUniform2f(PostEffects[I]['u_resolution'], GLfloat(Width), GLfloat(Height));
    glUniform1f(PostEffects[I]['u_gamma'], Gamma);
    if CustomUniforms <> Nil then CustomUniforms(PostEffects[I]);
    ScreenQuad.Draw(PostEffects[I], []);
  end;
end;

function TVoxScene.ExportOBJ(const DrawTypeBlacklist: TGLenumArray = Nil; InvertList: Boolean = False): String;
var
  ElementOffset: SizeInt = 1;
  MeshNumber: Integer = 1;
  MeshIsListed: Boolean;
  Mesh: TVoxMesh;
  I: Integer;
begin
  Result := '';
  for Mesh in Meshes do begin
    MeshIsListed := False;
    for I := 0 to Length(DrawTypeBlacklist) - 1 do begin
      if Mesh.DrawType <> DrawTypeBlacklist[I] then continue;
      MeshIsListed := True;
      break;
    end;
    if MeshIsListed <> InvertList then continue;
    Result += MeshExportOBJ(Mesh, Format('Mesh %d [%d]', [Mesh.ID, MeshNumber]), ElementOffset) + LineEnding;
    ElementOffset += Length(Mesh.Vertices);
    MeshNumber += 1;
  end;
end;

function TVoxScene.ToString(): String;
var
  I: Integer;
begin
  Result := Format('TVoxScene {', []) + LineEnding;
  Result += Format('  Width: %u,', [Width]) + LineEnding;
  Result += Format('  Height: %u,', [Height]) + LineEnding;
  Result += Format('  Samples: %d,', [Samples]) + LineEnding;
  Result += Format('  Gamma: %.3f,', [Gamma], VOX_FORMAT) + LineEnding;
  Result += Format('  ClearColor: %s,', [ClearColor.ToString()]) + LineEnding;
  Result += Format('  Camera: %s,', [Camera.ToString().Replace(LineEnding, LineEnding + '  ')]) + LineEnding;
  Result += Format('  Lamps: TVoxPhongLampList[%d] {', [Lamps.Count]) + LineEnding;
  for I := 0 to Lamps.Count - 1 do begin
    Result += '    ' + Lamps[I].ToString().Replace(LineEnding, LineEnding + '    ');
    if I < (Lamps.Count - 1) then Result += ',';
    Result += LineEnding;
  end;
  Result += Format('  },', []) + LineEnding;
  Result += Format('  ActiveRenderer: [%d],', [Renderers.IndexOf(ActiveRenderer)]) + LineEnding;
  Result += Format('  Renderers: TVoxRendererList[%d] {', [Renderers.Count]) + LineEnding;
  for I := 0 to Renderers.Count - 1 do begin
    Result += Format('    <%s>', [Renderers[I].ClassName]);
    if I < (Renderers.Count - 1) then Result += ',';
    Result += LineEnding;
  end;
  Result += Format('  },', []) + LineEnding;
  Result += Format('  PostEffects: <%d>,', [PostEffects.Count]) + LineEnding;
  Result += Format('}', []);
end;

{$endif}
