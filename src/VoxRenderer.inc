{interface}
{$ifdef INCLUDE_SECTION_INTERFACE}

type
  TVoxScene = class;
  TVoxFramebuffer = class;
  TVoxRenderer = class(TStoredErrorObject)
  protected
    Scene: TVoxScene;
    OldWidth, OldHeight: Integer;
    procedure SetMeshThreadLock(Locked: Boolean);
    procedure SetCommonUniforms(const Shader: TVoxShader);
    procedure DrawBackgroundGrid();
    procedure SetUniforms(const Shader: TVoxShader); virtual; abstract;
    procedure DoResize(NewWidth, NewHeight: Integer); virtual; abstract;
  public
    MainFramebuffer: TVoxFramebuffer;
    Wireframe: Boolean;
    constructor Create(const AScene: TVoxScene);
    destructor Destroy(); override;
    procedure Resize(NewWidth, NewHeight: Integer); virtual;
    procedure Draw(const TargetFramebuffer: TVoxFramebuffer); virtual; abstract;
  end;
  TVoxRendererDefault = class(TVoxRenderer)
  private
    MainShader: TVoxShader;
  protected
    procedure DoResize(NewWidth, NewHeight: Integer); override;
  public
    constructor Create(const AScene: TVoxScene);
    constructor Create(const AScene: TVoxScene; const ModelVertSource, ModelFragSource: String);
    destructor Destroy(); override;
    procedure SetUniforms(const Shader: TVoxShader); override;
    procedure Draw(const TargetFramebuffer: TVoxFramebuffer); override;
  end;
  TVoxRendererDDP = class(TVoxRenderer)
  private
    ScreenQuad: TVoxMesh;
    DDPFramebuffers: array[0..1] of TVoxFramebuffer;
    ShaderBackground, ShaderPeel, ShaderBackBlend, ShaderFinal: TVoxShader;
  protected
    procedure DoResize(NewWidth, NewHeight: Integer); override;
  public
    Passes: Integer;
    constructor Create(const AScene: TVoxScene);
    constructor Create(const AScene: TVoxScene; const ModelVertSource, BackgroundFragSource,
                       PeelingFragSource, BackBlendFragSource, FinalFragSource: String);
    destructor Destroy(); override;
    procedure SetUniforms(const Shader: TVoxShader); override;
    procedure Draw(const TargetFramebuffer: TVoxFramebuffer); override;
  end;
  TVoxRendererMeta = class(TVoxRenderer)
  private
    ShaderMeta: TVoxShader;
    function GetColorAttachment(): TVoxTexture; inline;
    function GetPositionAttachment(): TVoxTexture; inline;
    function GetIndexAttachment(): TVoxTexture; inline;
  protected
    procedure DoResize(NewWidth, NewHeight: Integer); override;
  public
    constructor Create(const AScene: TVoxScene);
    constructor Create(const AScene: TVoxScene; const ModelVertSource, MetaFragSource: String);
    destructor Destroy(); override;
    procedure SetUniforms(const Shader: TVoxShader); override;
    procedure Draw(const TargetFramebuffer: TVoxFramebuffer); override;
    procedure Draw(const TargetFramebuffer: TVoxFramebuffer;
                   const Blacklist: TVoxMeshArray; InvertList: Boolean = False);
    function PositionAt(X, Y: Integer): TVec3;
    function MeshIndexAt(X, Y: Integer): GLint;
    function MeshAt(X, Y: Integer): TVoxMesh;
    property ColorAttachment: TVoxTexture read GetColorAttachment;
    property PositionAttachment: TVoxTexture read GetPositionAttachment;
    property IndexAttachment: TVoxTexture read GetIndexAttachment;
  end;

{$endif}

{implementation}
{$ifdef INCLUDE_SECTION_IMPLEMENTATION}

// #########################################################
// ###                   TVoxRenderer                    ###
// #########################################################

procedure TVoxRenderer.SetMeshThreadLock(Locked: Boolean);
var
  Mesh: TVoxMesh;
begin
  for Mesh in Scene.Meshes do Mesh.MayBeFreed := not Locked;
end;

procedure TVoxRenderer.SetCommonUniforms(const Shader: TVoxShader);
var
  MatModel, MatView, MatProjection, MatLocalNormal, MatNormal, MatRayCast: TMatrix4;
  ArrayPrefix: String;
  I: Integer;
begin
  // Matrix uniforms
  MatModel := Scene.Camera.ModelMatrix();
  MatView := Scene.Camera.ViewMatrix();
  MatProjection := Scene.Camera.ProjectionMatrix();
  MatLocalNormal := MatModel.Inverse().Transpose();
  MatNormal := (MatView * MatModel).Inverse().Transpose();
  MatNormal[0][2] *= -1.0; MatNormal[1][2] *= -1.0; MatNormal[2][2] *= -1.0; // Invert normal Z
  MatRayCast := (MatProjection * MatView).Inverse(); // Permits firing rays into the scene through pixel coordinates
  glUniformMatrix4fv(Shader['u_mat_model'], 1, GL_FALSE, PGLfloat(@MatModel));
  glUniformMatrix4fv(Shader['u_mat_view'], 1, GL_FALSE, PGLfloat(@MatView));
  glUniformMatrix4fv(Shader['u_mat_projection'], 1, GL_FALSE, PGLfloat(@MatProjection));
  glUniformMatrix4fv(Shader['u_mat_local_normal'], 1, GL_FALSE, PGLfloat(@MatLocalNormal));
  glUniformMatrix4fv(Shader['u_mat_normal'], 1, GL_FALSE, PGLfloat(@MatNormal));
  glUniformMatrix4fv(Shader['u_mat_ray_cast'], 1, GL_FALSE, PGLfloat(@MatRayCast));
  // Lighting uniforms
  with (MatModel * Scene.Camera.Position) do glUniform3f(Shader['u_camera_pos'], X, Y, Z);
  with Scene.ClearColor.GammaCorrect(Scene.Gamma) do glUniform3f(Shader['u_clear_color'], R, G, B);
  glUniform1i(Shader['u_lamp_count'], Scene.Lamps.Count);
  for I := 0 to Scene.Lamps.Count - 1 do with Scene.Lamps[I] do begin
    ArrayPrefix := Format('u_lamps[%d].', [I]);
    glUniform1i(Shader[ArrayPrefix + 'mode'], Integer(Mode));
    with (MatModel * Position) do glUniform3f(Shader[ArrayPrefix + 'pos'], X, Y, Z);
    with Ambient do glUniform3f(Shader[ArrayPrefix + 'ambient'], R, G, B);
    with Diffuse do glUniform3f(Shader[ArrayPrefix + 'diffuse'], R, G, B);
    with Specular do glUniform3f(Shader[ArrayPrefix + 'specular'], R, G, B);
    glUniform1f(Shader[ArrayPrefix + 'constant'], Constant);
    glUniform1f(Shader[ArrayPrefix + 'linear'], Linear);
    glUniform1f(Shader[ArrayPrefix + 'quadratic'], Quadratic);
  end;
  // Other uniforms
  glUniform2f(Shader['u_resolution'], GLfloat(Scene.Width), GLfloat(Scene.Height));
  glUniform1f(Shader['u_gamma'], Scene.Gamma);
  glUniform1f(Shader['u_time'], Single(MillisecondsBetween(Now(), Scene.StartTimestamp)) / 1000.0);
end;

procedure TVoxRenderer.DrawBackgroundGrid();
begin
  if Length(Scene.RootMesh.Vertices) = 0 then Exit();
  glUseProgram(Scene.GridShader.ShaderProgram);
  SetCommonUniforms(Scene.GridShader);
  Scene.RootMesh.Draw(Scene.GridShader, []);
end;

constructor TVoxRenderer.Create(const AScene: TVoxScene);
begin
  Scene := AScene;
  OldWidth := Scene.Width;
  OldHeight := Scene.Height;
end;

destructor TVoxRenderer.Destroy();
begin
  MainFramebuffer.Free(); // Creation is handled in subclass
  inherited Destroy();
end;

procedure TVoxRenderer.Resize(NewWidth, NewHeight: Integer);
begin
  if (NewWidth <> OldWidth) or (NewHeight <> OldHeight) then DoResize(NewWidth, NewHeight);
  OldWidth := NewWidth;
  OldHeight := NewHeight;
end;

// #########################################################
// ###                TVoxRendererDefault                ###
// #########################################################

procedure TVoxRendererDefault.DoResize(NewWidth, NewHeight: Integer);
begin
  MainFramebuffer.Resize(NewWidth, NewHeight);
end;

constructor TVoxRendererDefault.Create(const AScene: TVoxScene);
begin
  Create(AScene, SHADER_VERT_MODEL, SHADER_FRAG_DEFAULT);
end;

constructor TVoxRendererDefault.Create(const AScene: TVoxScene; const ModelVertSource, ModelFragSource: String);
begin
  inherited Create(AScene);
  MainFramebuffer := TVoxFramebuffer.Create(
    Scene.Width, Scene.Height, [
      AttachmentInfo(GL_DEPTH_ATTACHMENT, TEX_FORM_DEPTH, GL_TEXTURE_2D_MULTISAMPLE),
      AttachmentInfo(GL_COLOR_ATTACHMENT0, TEX_FORM_COLOR, GL_TEXTURE_2D_MULTISAMPLE)
    ],
    Scene.Samples
  );
  MainShader := TVoxShader.Create(ModelVertSource, ModelFragSource);
  if not MainShader.IsValid then Error := MainShader.Error;
end;

destructor TVoxRendererDefault.Destroy();
begin
  MainShader.Free();
  inherited Destroy();
end;

procedure TVoxRendererDefault.SetUniforms(const Shader: TVoxShader);
begin
  SetCommonUniforms(Shader);
end;

procedure TVoxRendererDefault.Draw(const TargetFramebuffer: TVoxFramebuffer);
var
  Mesh: TVoxMesh;
begin
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_BLEND);
  // Draw to multisampled main framebuffer
  MainFramebuffer.Bind([0]);
  with Scene.ClearColor.GammaCorrect(Scene.Gamma) do glClearColor(R, G, B, A);
  glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);
  // Draw meshes
  glEnable(GL_LINE_SMOOTH);
  glPolygonMode(GL_FRONT_AND_BACK, IfThen(Wireframe, GL_LINE, GL_FILL));
  glUseProgram(MainShader.ShaderProgram);
  SetUniforms(MainShader);
  SetMeshThreadLock(True);
  for Mesh in Scene.Meshes do begin
    Mesh.Draw(MainShader, []);
  end;
  SetMeshThreadLock(False);
  DrawBackgroundGrid();
  // Draw result to target framebuffer
  MainFramebuffer.CopyTo(TargetFramebuffer, [0], [0]);
end;

// #########################################################
// ###                  TVoxRendererDDP                  ###
// #########################################################

procedure TVoxRendererDDP.DoResize(NewWidth, NewHeight: Integer);
begin
  MainFramebuffer.Resize(NewWidth, NewHeight);
  DDPFramebuffers[0].Resize(NewWidth, NewHeight);
  DDPFramebuffers[1].Resize(NewWidth, NewHeight);
end;

constructor TVoxRendererDDP.Create(const AScene: TVoxScene);
begin
  Create(AScene, SHADER_VERT_MODEL,
         SHADER_FRAG_DDP_BACKGROUND, SHADER_FRAG_DDP_PEEL, SHADER_FRAG_DDP_BACKBLEND, SHADER_FRAG_DDP_FINAL);
end;

constructor TVoxRendererDDP.Create(const AScene: TVoxScene; const ModelVertSource, BackgroundFragSource,
                                   PeelingFragSource, BackBlendFragSource, FinalFragSource: String);
const
  SCREEN_QUAD_TRIANGLES: array of TVec3 = (
    (X: -1; Y: -1; Z: 0), (X: 1; Y: -1; Z: 0), (X: -1; Y: 1; Z: 0),
    (X: 1; Y: 1; Z: 0), (X: -1; Y: 1; Z: 0), (X: 1; Y: -1; Z: 0)
  );
begin
  inherited Create(AScene);
  MainFramebuffer := TVoxFramebuffer.Create(
    Scene.Width, Scene.Height, [
      AttachmentInfo(GL_DEPTH_ATTACHMENT, TEX_FORM_DEPTH),
      AttachmentInfo(GL_COLOR_ATTACHMENT0, TEX_FORM_COLOR)
    ]
  );
  DDPFramebuffers[0] := TVoxFramebuffer.Create(
    Scene.Width, Scene.Height, [
      AttachmentInfo(GL_COLOR_ATTACHMENT0, TEX_FORM_DUAL_DEPTH),
      AttachmentInfo(GL_COLOR_ATTACHMENT1, TEX_FORM_COLOR),
      AttachmentInfo(GL_COLOR_ATTACHMENT2, TEX_FORM_COLOR)
    ]
  );
  DDPFramebuffers[1] := TVoxFramebuffer.Create(
    Scene.Width, Scene.Height, [
      AttachmentInfo(GL_COLOR_ATTACHMENT0, TEX_FORM_DUAL_DEPTH),
      AttachmentInfo(GL_COLOR_ATTACHMENT1, TEX_FORM_COLOR),
      AttachmentInfo(GL_COLOR_ATTACHMENT2, TEX_FORM_COLOR)
    ]
  );
  // Dual depth peeling-specific shaders
  ShaderBackground := TVoxShader.Create(ModelVertSource, BackgroundFragSource);
  if not ShaderBackground.IsValid then Error := ShaderBackground.Error;
  ShaderPeel := TVoxShader.Create(ModelVertSource, PeelingFragSource);
  if not ShaderPeel.IsValid then Error := ShaderPeel.Error;
  ShaderBackBlend := TVoxShader.Create(SHADER_VERT_SCREEN, BackBlendFragSource);
  if not ShaderBackBlend.IsValid then Error := ShaderBackBlend.Error;
  ShaderFinal := TVoxShader.Create(SHADER_VERT_SCREEN, FinalFragSource);
  if not ShaderFinal.IsValid then Error := ShaderFinal.Error;
  // Screen-filling quad and various other initializations
  ScreenQuad := TVoxMesh.Create(SCREEN_QUAD_TRIANGLES);
  Passes := 4;
end;

destructor TVoxRendererDDP.Destroy();
begin
  DDPFramebuffers[0].Free();
  DDPFramebuffers[1].Free();
  ScreenQuad.Free();
  ShaderBackground.Free();
  ShaderPeel.Free();
  ShaderBackBlend.Free();
  ShaderFinal.Free();
  inherited Destroy();
end;

procedure TVoxRendererDDP.SetUniforms(const Shader: TVoxShader);
begin
  SetCommonUniforms(Shader);
end;

procedure TVoxRendererDDP.Draw(const TargetFramebuffer: TVoxFramebuffer);
var
  Mesh: TVoxMesh;
  ReadIndex: Integer;
  WriteIndex: Integer = 0;
  I: Integer;
begin
  // Clear final color buffer
  MainFramebuffer.Bind([0]);
  with Scene.ClearColor.GammaCorrect(Scene.Gamma) do glClearColor(R, G, B, A);
  glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);  // MainFramebuffer's depth buffer is used later
  // Intialize min-max depth buffer
  DDPFramebuffers[0].Bind([0]);
  glClearColor(-Infinity, -Infinity, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  DDPFramebuffers[1].Bind([0]);
  glClearColor(0.0, 1.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  // Clear color accumulator buffers
  for I := 0 to 1 do begin
    DDPFramebuffers[I].Bind([1, 2]);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
  end;
  // Draw grid to background and render "regular" depth
  glEnable(GL_DEPTH_TEST);    // This step serves to generate an as-expected depth buffer for potential further use
  glDepthFunc(GL_LEQUAL);     // It also draws the scene grid, if desired
  glDisable(GL_BLEND);
  glDisable(GL_LINE_SMOOTH);  // Antialiased lines don't seem to play well with this technique
  glPolygonMode(GL_FRONT_AND_BACK, IfThen(Wireframe, GL_LINE, GL_FILL));
  MainFramebuffer.Bind([0]);  // ShaderBackground only serves to render depth and discard fully transparent fragments
  glUseProgram(ShaderBackground.ShaderProgram);
  SetUniforms(ShaderBackground);
  SetMeshThreadLock(True);
  for Mesh in Scene.Meshes do begin
    Mesh.Draw(ShaderBackground, []);
  end;
  SetMeshThreadLock(False);
  glDisable(GL_DEPTH_TEST); // This is required both by the next line and the following dual-depth-peeling algorithm
  DrawBackgroundGrid();     // Draw grid to background (other meshes will always overdraw it, requiring a second step)
  // Draw depth for first pass
  glEnable(GL_BLEND);
  glBlendEquationEXT(GL_MAX);
  DDPFramebuffers[0].Bind([0]);
  glUseProgram(ShaderPeel.ShaderProgram); // Peeling
  SetUniforms(ShaderPeel);
  SetMeshThreadLock(True);
  for Mesh in Scene.Meshes do Mesh.Draw(ShaderPeel, [
    DDPFramebuffers[1].ColorAttachments[0], // Depth
    DDPFramebuffers[1].ColorAttachments[1]  // Front
  ]);
  SetMeshThreadLock(False);
  // Dual depth peeling passes
  for I := 0 to Passes - 1 do begin
    ReadIndex := I mod 2;
    WriteIndex := 1 - ReadIndex;
    // Clear min-max depth buffer (w)
    DDPFramebuffers[WriteIndex].Bind([0]);
    glClearColor(-Infinity, -Infinity, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Clear color buffer (w)
    DDPFramebuffers[WriteIndex].Bind([1, 2]);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Set up rendering (w), update textures and draw
    DDPFramebuffers[WriteIndex].Bind([0, 1, 2]);
    glPolygonMode(GL_FRONT_AND_BACK, IfThen(Wireframe, GL_LINE, GL_FILL));
    glUseProgram(ShaderPeel.ShaderProgram); // Peeling
    glBlendEquationEXT(GL_MAX);
    SetMeshThreadLock(True);
    for Mesh in Scene.Meshes do Mesh.Draw(ShaderPeel, [
      DDPFramebuffers[ReadIndex].ColorAttachments[0], // Depth
      DDPFramebuffers[ReadIndex].ColorAttachments[1]  // Front
    ]);
    SetMeshThreadLock(False);
    // Blend back color
    MainFramebuffer.Bind([0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBlendEquationEXT(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(ShaderBackBlend.ShaderProgram); // Back blend
    SetUniforms(ShaderBackBlend);
    ScreenQuad.Draw(ShaderBackBlend, [DDPFramebuffers[WriteIndex].ColorAttachments[2]]); // Back
  end;
  // Final composition
  TargetFramebuffer.Bind([0]);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glUseProgram(ShaderFinal.ShaderProgram); // DDP final
  SetUniforms(ShaderFinal);
  ScreenQuad.Draw(ShaderFinal, [
    DDPFramebuffers[WriteIndex].ColorAttachments[1],
    MainFramebuffer.ColorAttachments[0]
  ]);
  MainFramebuffer.CopyTo(TargetFramebuffer, [], [], True); // Copy previously drawn "regular" depth buffer to target
  glEnable(GL_DEPTH_TEST);  // The first DrawBackgroundDepth call didn't write its own DDP-compatible depth before;
  DrawBackgroundGrid();     // hence why it was wrongly overdrawn by meshes behind it. Solution: redraw with depth test
end;

// #########################################################
// ###                 TVoxRendererMeta                  ###
// #########################################################

function TVoxRendererMeta.GetColorAttachment(): TVoxTexture;
begin
  Result := MainFramebuffer.ColorAttachments[0];
end;

function TVoxRendererMeta.GetPositionAttachment(): TVoxTexture;
begin
  Result := MainFramebuffer.ColorAttachments[1];
end;

function TVoxRendererMeta.GetIndexAttachment(): TVoxTexture;
begin
  Result := MainFramebuffer.ColorAttachments[2];
end;

procedure TVoxRendererMeta.DoResize(NewWidth, NewHeight: Integer);
begin
  MainFramebuffer.Resize(NewWidth, NewHeight);
end;

constructor TVoxRendererMeta.Create(const AScene: TVoxScene);
begin
  Create(AScene, SHADER_VERT_MODEL, SHADER_FRAG_META);
end;

constructor TVoxRendererMeta.Create(const AScene: TVoxScene; const ModelVertSource, MetaFragSource: String);
begin
  inherited Create(AScene);
  MainFramebuffer := TVoxFramebuffer.Create(
    Scene.Width, Scene.Height, [
      AttachmentInfo(GL_DEPTH_ATTACHMENT, TEX_FORM_DEPTH),
      AttachmentInfo(GL_COLOR_ATTACHMENT0, TEX_FORM_COLOR),
      AttachmentInfo(GL_COLOR_ATTACHMENT1, TEX_FORM_POSITION),
      AttachmentInfo(GL_COLOR_ATTACHMENT2, TEX_FORM_INDEX)
    ]
  );
  ShaderMeta := TVoxShader.Create(ModelVertSource, MetaFragSource);
  if not ShaderMeta.IsValid then Error := ShaderMeta.Error;
end;

destructor TVoxRendererMeta.Destroy();
begin
  ShaderMeta.Free();
  inherited Destroy();
end;

procedure TVoxRendererMeta.SetUniforms(const Shader: TVoxShader);
const
  COLORMAP: array of TColor4 = ( // "Turbo" colormap
    (R: 0.302; G: 0.431; B: 0.875; A: 1.000), (R: 0.239; G: 0.725; B: 0.914; A: 1.000),
    (R: 0.267; G: 0.933; B: 0.604; A: 1.000), (R: 0.643; G: 0.980; B: 0.314; A: 1.000),
    (R: 0.922; G: 0.808; B: 0.298; A: 1.000), (R: 0.969; G: 0.506; B: 0.216; A: 1.000),
    (R: 0.808; G: 0.227; B: 0.125; A: 1.000)
  );
begin
  SetCommonUniforms(Shader);
  glUniform1i(Shader['u_colormap_count'], Length(COLORMAP));
  glUniform4fv(Shader['u_colormap[0]'], Length(COLORMAP), @COLORMAP[0]);
end;

procedure TVoxRendererMeta.Draw(const TargetFramebuffer: TVoxFramebuffer);
begin
  Draw(TargetFramebuffer, Nil, False);
end;

procedure TVoxRendererMeta.Draw(const TargetFramebuffer: TVoxFramebuffer;
                                const Blacklist: TVoxMeshArray; InvertList: Boolean = False);
const
  INDEX_CLEAR_COLOR: array[0..3] of GLint = (-1, -1, -1, -1);
var
  Mesh: TVoxMesh;
  I: Integer;
begin
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_BLEND);
  // Draw to meta framebuffer
  MainFramebuffer.Bind([0, 1, 2]); // Color, position, id
  glClearColor(NaN, NaN, NaN, 1.0);
  glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);
  glClearBufferiv(GL_COLOR, 2, @INDEX_CLEAR_COLOR[0]);
  // Draw meshes
  glDisable(GL_LINE_SMOOTH);
  glPolygonMode(GL_FRONT_AND_BACK, IfThen(Wireframe, GL_LINE, GL_FILL));
  glUseProgram(ShaderMeta.ShaderProgram);
  SetUniforms(ShaderMeta);
  SetMeshThreadLock(True);
  for I := 0 to Scene.Meshes.Count - 1 do begin
    Mesh := Scene.Meshes[I];
    if (Blacklist.IndexOf(Mesh) >= 0) xor InvertList then continue;
    case Mesh.MetaType of
      mtPassive: continue;
      else Mesh.Draw(ShaderMeta, [], I);
    end;
  end;
  SetMeshThreadLock(False);
  // Draw result to target framebuffer
  if TargetFramebuffer <> Nil then MainFramebuffer.CopyTo(TargetFramebuffer, [0], [0]);
end;

function TVoxRendererMeta.PositionAt(X, Y: Integer): TVec3;
begin
  glBindFramebuffer(GL_FRAMEBUFFER, MainFramebuffer.FBO);
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  with MainFramebuffer.ColorAttachments[1].Format do begin
    glReadPixels(X, Scene.Height - Y - 1, 1, 1, PixelFormat, PixelType, @Result);
  end;
  Result := Scene.Camera.ModelMatrix().Inverse() * Result; // Convert from local coordinates to world coordinates
end;

function TVoxRendererMeta.MeshIndexAt(X, Y: Integer): GLint;
begin
  glBindFramebuffer(GL_FRAMEBUFFER, MainFramebuffer.FBO);
  glReadBuffer(GL_COLOR_ATTACHMENT2);
  with MainFramebuffer.ColorAttachments[2].Format do begin
    glReadPixels(X, Scene.Height - Y - 1, 1, 1, PixelFormat, PixelType, @Result);
  end;
end;

function TVoxRendererMeta.MeshAt(X, Y: Integer): TVoxMesh;
var
  Index: GLint;
begin
  Index := MeshIndexAt(X, Y);
  if (Index >= 0) and (Index < Scene.Meshes.Count) then begin
    Result := Scene.Meshes[Index];
  end else Result := Nil;
end;

{$endif}
