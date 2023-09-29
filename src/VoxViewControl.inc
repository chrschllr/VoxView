{interface}
{$ifdef INCLUDE_SECTION_INTERFACE}

type
  TSmoothDelay = record
  private
    TimeStart: Int64;
    IsUnread: Boolean;
    PastValue: Double;
    FValueStart, FValueEnd: Double;
    procedure SetStartValue(AValueStart: Double);
    procedure SetEndValue(AValueEnd: Double);
    function GetValue(): Double;
    function GetDiffValue(): Double;
    function GetStatus(): Boolean;
  public
    SettlingTime: Integer;
    property ValueStart: Double read FValueStart write SetStartValue;
    property ValueEnd: Double read FValueEnd write SetEndValue;
    property Value: Double read GetValue;
    property DiffValue: Double read GetDiffValue;
    property IsDone: Boolean read GetStatus;
  end;
  TVoxViewSide = (vsRight, vsFront, vsTop, vsLeft, vsBack, vsBottom, vsIsometric);
  TVoxViewFrameEvent = procedure(Sender: TObject; Millis: Int64; DT: Double) of object;
  TVoxView = class(TOpenGLControl)
  const
    DEFAULT_FPS = 60;
    DEFAULT_IS_PAUSED = False;
    DEFAULT_ALLOW_THROTTLE = True;
    DEFAULT_ORBIT_SPEED = 100;
    DEFAULT_ORBIT_MOUSE_COMBO = [ssLeft];
    DEFAULT_ZOOM_SPEED = 100;
    DEFAULT_ZOOM_INVERT = False;
    DEFAULT_ZOOM_MOUSE_COMBO = [];
    DEFAULT_DOLLY_SPEED = 100;
    DEFAULT_DOLLY_INVERT = False;
    DEFAULT_DOLLY_MOUSE_COMBO = [ssCtrl, ssMiddle];
    DEFAULT_PAN_SPEED = 100;
    DEFAULT_PAN_MOUSE_COMBO = [ssCtrl, ssLeft];
    DEFAULT_SELECT_MOUSE_COMBO = [ssShift, ssLeft];
    DEFAULT_SMOOTH_MOVE_TIME = 20;
    DEFAULT_SMOOTH_SCROLL_TIME = 100;
    DEFAULT_OUTLINE_SELECTION = False;
    DEFAULT_OUTLINE_SELECTION_WIDTH = 3;
    DEFAULT_OUTLINE_SELECTION_COLOR = TColor($28BFFF);
    DEFAULT_SKY_COLOR = TColor($322821);
    DEFAULT_GRID_STANDARD_COLOR = TColor($808080);
    DEFAULT_GRID_STANDARD_ALPHA = 25;
    DEFAULT_GRID_STANDARD_RADIUS = 1000;
    DEFAULT_STANDARD_PASSES = 4;
    DEFAULT_AUTO_RESIZE_VIEWPORT = True;
    DEFAULT_OPENGL_MAJOR_VERSION = 3;
    DEFAULT_OPENGL_MINOR_VERSION = 3;
    DEFAULT_MULTISAMPLING = 4;
  private
    PausedTimestamp, TotalPausedTime: Int64;
    FrameTimestamps: array[0..299] of Int64;
    LastEffectiveTime: Int64;
    FrameSleepTime: Integer;
    FOnFrame: TVoxViewFrameEvent;
    FFPS: Integer;
    FAllowThrottle: Boolean;
    FIsPaused: Boolean;
    FOrbitSpeed, FZoomSpeed, FDollySpeed, FPanSpeed: Integer;
    FOrbitMouseCombo, FZoomMouseCombo, FDollyMouseCombo, FPanMouseCombo, FSelectMouseCombo: TShiftState;
    FZoomInvert, FDollyInvert: Boolean;
    FOutlineSelection: Boolean;
    FOutlineSelectionWidth: Integer;
    FOutlineSelectionColor: TColor;
    FSkyColor: TColor;
    FGridStandardColor: TColor;
    FGridStandardAlpha: Integer;
    FGridStandardRadius: Cardinal;
    FStandardPasses: Integer;
    FSelectedMesh: TVoxMesh;
    IsContextInitialized: Boolean;
    IsOrbitTranspolar: Boolean;
    IsInactive, IsMinimized: Boolean;
    Mouse: record
      Shift: TShiftState;
      Smooth: record
        X, Y, W: TSmoothDelay;
      end;
    end;
    Pivot: TVec3;
    function IsComponentReady(): Boolean;
    procedure SetPaused(AIsPaused: Boolean);
    procedure SetSmoothMoveTime(ADelay: Integer);
    function GetSelectedMesh(): TVoxMesh;
    procedure SetSelectedMesh(const ASelectedMesh: TVoxMesh);
    procedure SetViewerUniforms(const Shader: TVoxShader);
    procedure FindNewPivot(ClickX, ClickY: Integer);
    procedure OnApplicationIdle({%H-}Sender: TObject; var Done: Boolean);
    procedure OnApplicationActivate({%H-}Sender: TObject);
    procedure OnApplicationDeactivate({%H-}Sender: TObject);
    procedure OnApplicationRestore({%H-}Sender: TObject);
    procedure OnApplicationMinimize({%H-}Sender: TObject);
    procedure MouseMoveDiff(Shift: TShiftState; DX, DY: Double);
    procedure MouseWheelDiff(Shift: TShiftState; DW: Double);
  protected
    procedure MouseDown(Button: TMouseButton; Shift: TShiftState; X, Y: Integer); override;
    procedure MouseUp(Button: TMouseButton; Shift: TShiftState; X, Y: Integer); override;
    procedure MouseMove(Shift: TShiftState; X, Y: Integer); override;
    function DoMouseWheel(Shift: TShiftState; WheelDelta: Integer; MousePos: TPoint): Boolean; override;
  public
    Scene: TVoxScene;
    OutlineShader: TVoxShader;
    constructor Create(TheOwner: TComponent); override;
    destructor Destroy(); override;
    function MakeCurrent(SaveOldToStack: Boolean = False): Boolean; reintroduce;
    procedure Resize(); override;
    procedure DoOnPaint(); override;
    function MeasureFPS(Interval: Integer = 500; MinSamples: Integer = 2): Double;
    procedure SelectMeshAt(X, Y: Integer);
    procedure FitVisibleToScreen(Margin: Double = 0.0);
    procedure SetProjection(Projection: TVoxProjection);
    procedure ViewSide(Side: TVoxViewSide; Margin: Double = 0.0);
    procedure NavigateOrbit(DX, DY: Double);
    procedure NavigateZoom(DW: Double);
    procedure NavigateDolly(DY: Double);
    procedure NavigatePan(DX, DY: Double);
    property SelectedMesh: TVoxMesh read GetSelectedMesh write SetSelectedMesh;
  published
    property FPS: Integer read FFPS write FFPS default DEFAULT_FPS;
    property AllowThrottle: Boolean read FAllowThrottle write FAllowThrottle default DEFAULT_ALLOW_THROTTLE;
    property IsPaused: Boolean read FIsPaused write SetPaused default DEFAULT_IS_PAUSED;
    property OrbitSpeed: Integer read FOrbitSpeed write FOrbitSpeed default DEFAULT_ORBIT_SPEED;
    property OrbitMouseCombo: TShiftState read FOrbitMouseCombo write FOrbitMouseCombo
                                          default DEFAULT_ORBIT_MOUSE_COMBO;
    property ZoomSpeed: Integer read FZoomSpeed write FZoomSpeed default DEFAULT_ZOOM_SPEED;
    property ZoomInvert: Boolean read FZoomInvert write FZoomInvert default DEFAULT_ZOOM_INVERT;
    property ZoomMouseCombo: TShiftState read FZoomMouseCombo write FZoomMouseCombo default DEFAULT_ZOOM_MOUSE_COMBO;
    property DollySpeed: Integer read FDollySpeed write FDollySpeed default DEFAULT_DOLLY_SPEED;
    property DollyInvert: Boolean read FDollyInvert write FDollyInvert default DEFAULT_DOLLY_INVERT;
    property DollyMouseCombo: TShiftState read FDollyMouseCombo write FDollyMouseCombo
                                          default DEFAULT_Dolly_MOUSE_COMBO;
    property PanSpeed: Integer read FPanSpeed write FPanSpeed default DEFAULT_PAN_SPEED;
    property PanMouseCombo: TShiftState read FPanMouseCombo write FPanMouseCombo default DEFAULT_PAN_MOUSE_COMBO;
    property SelectMouseCombo: TShiftState read FSelectMouseCombo write FSelectMouseCombo
                                           default DEFAULT_SELECT_MOUSE_COMBO;
    property SmoothMoveTime: Integer read Mouse.Smooth.X.SettlingTime write SetSmoothMoveTime
                                     default DEFAULT_SMOOTH_MOVE_TIME;
    property SmoothScrollTime: Integer read Mouse.Smooth.W.SettlingTime write Mouse.Smooth.W.SettlingTime
                                       default DEFAULT_SMOOTH_SCROLL_TIME;
    property OutlineSelection: Boolean read FOutlineSelection write FOutlineSelection
                                       default DEFAULT_OUTLINE_SELECTION;
    property OutlineSelectionWidth: Integer read FOutlineSelectionWidth write FOutlineSelectionWidth
                                            default DEFAULT_OUTLINE_SELECTION_WIDTH;
    property OutlineSelectionColor: TColor read FOutlineSelectionColor write FOutlineSelectionColor
                                           default DEFAULT_OUTLINE_SELECTION_COLOR;
    property SkyColor: TColor read FSkyColor write FSkyColor default DEFAULT_SKY_COLOR;
    property GridStandardColor: TColor read FGridStandardColor write FGridStandardColor
                                       default DEFAULT_GRID_STANDARD_COLOR;
    property GridStandardAlpha: Integer read FGridStandardAlpha write FGridStandardAlpha
                                        default DEFAULT_GRID_STANDARD_ALPHA;
    property GridStandardRadius: Cardinal read FGridStandardRadius write FGridStandardRadius
                                          default DEFAULT_GRID_STANDARD_RADIUS;
    property StandardPasses: Integer read FStandardPasses write FStandardPasses default DEFAULT_STANDARD_PASSES;
    property OnFrame: TVoxViewFrameEvent read FOnFrame write FOnFrame;
    property AutoResizeViewport default DEFAULT_AUTO_RESIZE_VIEWPORT;
    property OpenGLMajorVersion default DEFAULT_OPENGL_MAJOR_VERSION;
    property OpenGLMinorVersion default DEFAULT_OPENGL_MINOR_VERSION;
    property MultiSampling default DEFAULT_MULTISAMPLING;
  end;

procedure Register();
function MillisecondTimer(): Int64;

{$endif}

{implementation}
{$ifdef INCLUDE_SECTION_IMPLEMENTATION}

procedure Register();
begin
  {$i VoxViewIcon.lrs}
  RegisterComponents('OpenGL', [TVoxView]);
end;

{$ifdef Windows}

function MillisecondTimer(): Int64;
const
  CounterFreq: Int64 = 0;
begin
  Result := 0;
  QueryPerformanceCounter(Result); // Won't overflow unless the machine has been continuosly running for ~60,000 years
  if CounterFreq = 0 then QueryPerformanceFrequency({%H-}CounterFreq);
  Result := 1000 * Result div CounterFreq;
end;

{$else}

function MillisecondTimer(): Int64;
var
  Time: TTimeSpec;
begin
  if clock_gettime(CLOCK_MONOTONIC, @Time) <> 0 then Exit(GetTickCount64());
  Result := Time.tv_sec * 1000 + Time.tv_nsec div 1000000;
end;

{$endif}

// #########################################################
// ###                   TSmoothDelay                    ###
// #########################################################

procedure TSmoothDelay.SetStartValue(AValueStart: Double);
begin
  TimeStart := 0;
  IsUnread := True;
  FValueStart := AValueStart;
  FValueEnd := AValueStart;
  PastValue := AValueStart;
end;

procedure TSmoothDelay.SetEndValue(AValueEnd: Double);
begin
  IsUnread := True;
  FValueStart := Value;
  FValueEnd := AValueEnd;
  TimeStart := MillisecondTimer();
end;

function TSmoothDelay.GetValue(): Double;
var
  Factor: Double;
begin
  if SettlingTime = 0.0 then Exit(ValueEnd);
  Factor := (MillisecondTimer() - TimeStart) / SettlingTime;
  if Factor > 1.0 then Factor := 1.0;
  Result := (1.0 - Factor) * ValueStart + Factor * ValueEnd;
end;

function TSmoothDelay.GetDiffValue(): Double;
var
  CurrentValue: Double;
begin
  CurrentValue := Value;
  Result := CurrentValue - PastValue;
  PastValue := CurrentValue;
end;

function TSmoothDelay.GetStatus(): Boolean;
begin
  Result := ((MillisecondTimer() - TimeStart) >= SettlingTime) and not IsUnread;
  IsUnread := False;
end;

// #########################################################
// ###                     TVoxView                      ###
// #########################################################

function TVoxView.IsComponentReady(): Boolean;
begin
  Result := [csLoading, csDestroying, csDesigning] * ComponentState = [];
end;

procedure TVoxView.SetPaused(AIsPaused: Boolean);
begin
  if FIsPaused <> AIsPaused then begin
    if AIsPaused then PausedTimestamp := MillisecondTimer()           // Pause
    else TotalPausedTime += MillisecondTimer() - PausedTimestamp;     // Unpause
  end;
  FIsPaused := AIsPaused;
end;

procedure TVoxView.SetSmoothMoveTime(ADelay: Integer);
begin
  Mouse.Smooth.X.SettlingTime := ADelay;
  Mouse.Smooth.Y.SettlingTime := ADelay;
end;

function TVoxView.GetSelectedMesh(): TVoxMesh;
begin
  if not Scene.Meshes.Contains(FSelectedMesh) then FSelectedMesh := Nil;
  Result := FSelectedMesh;
end;

procedure TVoxView.SetSelectedMesh(const ASelectedMesh: TVoxMesh);
begin
  FSelectedMesh := ASelectedMesh;
  if not Scene.Meshes.Contains(FSelectedMesh) then FSelectedMesh := Nil;
end;

procedure TVoxView.SetViewerUniforms(const Shader: TVoxShader);
begin
  glUniform1i(Shader['u_outline_mesh_index'], Scene.Meshes.IndexOf(SelectedMesh));
  glUniform1f(Shader['u_outline_width'], GLfloat(OutlineSelectionWidth));
  with TColor4(OutlineSelectionColor) do glUniform3f(Shader['u_outline_color'], R, G, B);
end;

procedure TVoxView.FindNewPivot(ClickX, ClickY: Integer);
var
  PointedPosition: TVec3;
begin
  Scene.MetaRenderer.Draw(Nil);
  PointedPosition := Scene.MetaRenderer.PositionAt(ClickX, ClickY);
  if not PointedPosition.IsNaN() then Pivot := PointedPosition;
end;

procedure TVoxView.OnApplicationIdle(Sender: TObject; var Done: Boolean);
var
  FrameEffectiveTime: Int64;
begin
  Done := IsPaused or IsMinimized or not IsComponentReady();
  if Done then Exit();
  if AllowThrottle and IsInactive then Sleep(100); // Save resources when not actively being used
  if (MillisecondTimer() - FrameTimestamps[0]) < FrameSleepTime then begin
    if FrameSleepTime > 0 then Sleep(FrameSleepTime);
    Exit();
  end;
  FrameEffectiveTime := MillisecondTimer() - TotalPausedTime;
  if OnFrame <> Nil then OnFrame(Self, FrameEffectiveTime, (FrameEffectiveTime - LastEffectiveTime) / 1000.0);
  with Mouse.Smooth do if not (X.IsDone and Y.IsDone) then MouseMoveDiff(Mouse.Shift, X.DiffValue, Y.DiffValue);
  with Mouse.Smooth do if not W.IsDone then MouseWheelDiff(Mouse.Shift, W.DiffValue);
  Paint();
  FrameSleepTime += Trunc(1000.0 * (1.0 / FPS - 1.0 / MeasureFPS(0, 4))); // Take the last 4 frames into account
  FrameSleepTime := Min(Max(FrameSleepTime, -100), 200); // Try not to drop much below 5 FPS (200 ms)
  LastEffectiveTime := FrameEffectiveTime;
end;

procedure TVoxView.OnApplicationActivate(Sender: TObject);
begin
  IsInactive := False;
end;

procedure TVoxView.OnApplicationDeactivate(Sender: TObject);
begin
  IsInactive := True;
end;

procedure TVoxView.OnApplicationRestore(Sender: TObject);
begin
  IsMinimized := False;
end;

procedure TVoxView.OnApplicationMinimize(Sender: TObject);
begin
  IsMinimized := True;
end;

procedure TVoxView.MouseMoveDiff(Shift: TShiftState; DX, DY: Double);
begin
  if Shift = OrbitMouseCombo then NavigateOrbit(DX, DY);
  if Shift = PanMouseCombo then NavigatePan(DX, DY);
  if Shift = DollyMouseCombo then NavigateDolly(DY * IfThen(DollyInvert, -1, 1));
end;

procedure TVoxView.MouseWheelDiff(Shift: TShiftState; DW: Double);
begin
  if Shift = ZoomMouseCombo then NavigateZoom(DW * IfThen(ZoomInvert, -1, 1));
end;

procedure TVoxView.MouseDown(Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  FindNewPivot(X, Y);
  if Shift = SelectMouseCombo then SelectMeshAt(X, Y);
  Mouse.Smooth.X.ValueStart := X;
  Mouse.Smooth.Y.ValueStart := Y;
  Mouse.Shift := Shift;
  MouseCapture := True;
  inherited MouseDown(Button, Shift, X, Y);
end;

procedure TVoxView.MouseUp(Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  Mouse.Shift := Shift;
  MouseCapture := False;
  inherited MouseUp(Button, Shift, X, Y);
end;

procedure TVoxView.MouseMove(Shift: TShiftState; X, Y: Integer);
begin
  Mouse.Smooth.X.ValueEnd := X;
  Mouse.Smooth.Y.ValueEnd := Y;
  inherited MouseMove(Shift, X, Y);
  // Intentionally do not assign Mouse.Shift here
end;

function TVoxView.DoMouseWheel(Shift: TShiftState; WheelDelta: Integer; MousePos: TPoint): Boolean;
begin
  FindNewPivot(MousePos.X, MousePos.Y);
  Mouse.Smooth.W.ValueEnd := Mouse.Smooth.W.ValueEnd + WheelDelta;
  Mouse.Shift := Shift;
  Result := inherited DoMouseWheel(Shift, WheelDelta, MousePos);
end;

constructor TVoxView.Create(TheOwner: TComponent);
begin
  inherited Create(TheOwner);
  FPS := DEFAULT_FPS;
  IsPaused := DEFAULT_IS_PAUSED;
  AllowThrottle := DEFAULT_ALLOW_THROTTLE;
  OrbitSpeed := DEFAULT_ORBIT_SPEED;
  OrbitMouseCombo := DEFAULT_ORBIT_MOUSE_COMBO;
  ZoomSpeed := DEFAULT_ZOOM_SPEED;
  ZoomInvert := DEFAULT_ZOOM_INVERT;
  ZoomMouseCombo := DEFAULT_ZOOM_MOUSE_COMBO;
  DollySpeed := DEFAULT_DOLLY_SPEED;
  DollyInvert := DEFAULT_DOLLY_INVERT;
  DollyMouseCombo := DEFAULT_DOLLY_MOUSE_COMBO;
  PanSpeed := DEFAULT_PAN_SPEED;
  PanMouseCombo := DEFAULT_PAN_MOUSE_COMBO;
  SelectMouseCombo := DEFAULT_SELECT_MOUSE_COMBO;
  SmoothMoveTime := DEFAULT_SMOOTH_MOVE_TIME;
  SmoothScrollTime := DEFAULT_SMOOTH_SCROLL_TIME;
  OutlineSelection := DEFAULT_OUTLINE_SELECTION;
  OutlineSelectionWidth := DEFAULT_OUTLINE_SELECTION_WIDTH;
  OutlineSelectionColor := DEFAULT_OUTLINE_SELECTION_COLOR;
  SkyColor := DEFAULT_SKY_COLOR;
  GridStandardColor := DEFAULT_GRID_STANDARD_COLOR;
  GridStandardAlpha := DEFAULT_GRID_STANDARD_ALPHA;
  GridStandardRadius := DEFAULT_GRID_STANDARD_RADIUS;
  StandardPasses := DEFAULT_STANDARD_PASSES;
  AutoResizeViewport := DEFAULT_AUTO_RESIZE_VIEWPORT;
  OpenGLMajorVersion := DEFAULT_OPENGL_MAJOR_VERSION;
  OpenGLMinorVersion := DEFAULT_OPENGL_MINOR_VERSION;
  MultiSampling := DEFAULT_MULTISAMPLING;
  if not IsComponentReady() then Exit();
  TotalPausedTime := MillisecondTimer();
  Application.AddOnIdleHandler(@OnApplicationIdle, False); // Constant-framerate rendering event
  Application.AddOnActivateHandler(@OnApplicationActivate, False);
  Application.AddOnDeactivateHandler(@OnApplicationDeactivate, False);
  application.AddOnRestoreHandler(@OnApplicationRestore, False);
  application.AddOnMinimizeHandler(@OnApplicationMinimize, False);
end;

destructor TVoxView.Destroy();
begin
  if [csLoading, csDesigning] * ComponentState = [] then begin
    Application.RemoveOnIdleHandler(@OnApplicationIdle);
    Application.RemoveOnActivateHandler(@OnApplicationActivate);
    Application.RemoveOnDeactivateHandler(@OnApplicationDeactivate);
    Application.RemoveOnRestoreHandler(@OnApplicationRestore);
    Application.RemoveOnMinimizeHandler(@OnApplicationMinimize);
    Scene.PostEffects.Remove(OutlineShader);
    OutlineShader.Free();
    Scene.Free();
  end;
  inherited Destroy();
end;

function TVoxView.MakeCurrent(SaveOldToStack: Boolean = False): Boolean;
var
  LoadedGL33, LoadedBlendMinMax: Boolean;
begin
  if not IsComponentReady() then Exit(False);
  Result := inherited MakeCurrent(SaveOldToStack);
  if IsContextInitialized then Exit();
  LoadedGL33 := Load_GL_VERSION_3_3();
  {$ifdef Windows}
  LoadedBlendMinMax := Load_GL_EXT_blend_minmax();
  {$else} // Bypass extension support check on Linux, TODO: find out why not all glxinfo extensions are listed
  Pointer(glBlendEquationEXT) := wglGetProcAddress('glBlendEquationEXT');
  LoadedBlendMinMax := glBlendEquationEXT <> Nil;
  {$endif}
  Scene := TVoxScene.Create(Width, Height, MultiSampling);
  Scene.SetGrid(GridStandardRadius, Color4(TColor4(GridStandardColor), GridStandardAlpha / 255));
  if LoadedBlendMinMax then begin
    Scene.ActiveRenderer := TVoxRendererDDP.Create(Scene);
    TVoxRendererDDP(Scene.ActiveRenderer).Passes := StandardPasses;
  end;
  OutlineShader := TVoxShader.Create(SHADER_VERT_SCREEN, SHADER_FRAG_OUTLINE);
  IsContextInitialized := True;
  if not (LoadedGL33 and LoadedBlendMinMax) then begin
    IsPaused := True;
    raise Exception.CreateFmt('(VoxView) This machine doesn''t meet the following requirements: %s', [
      IfThen(not LoadedGL33, LineEnding + '- OpenGL 3.3', '') +
      IfThen(not LoadedBlendMinMax, LineEnding + '- GL_EXT_blend_minmax', '')
    ]);
  end;
end;

procedure TVoxView.Resize();
begin
  if Scene <> Nil then Scene.Resize(Width, Height);
  inherited Resize();
end;

procedure TVoxView.DoOnPaint();
var
  OutlineFX: Integer = -1;
begin
  if not MakeCurrent() then Exit();
  Move(FrameTimestamps[0], FrameTimestamps[1], SizeOf(FrameTimestamps[0]) * (Length(FrameTimestamps) - 1));
  FrameTimestamps[0] := MillisecondTimer(); // Push latest timestamp
  if OnPaint <> Nil then OnPaint(Self);
  if OutlineSelection and (SelectedMesh <> Nil) then begin
    OutlineFX := Scene.PostEffects.Insert(1, OutlineShader); // Execute before standard FXAA
    Scene.MetaRenderer.Draw(Nil, [SelectedMesh], True);
  end;
  Scene.ClearColor := TColor4(SkyColor);
  Scene.Draw(@SetViewerUniforms);
  if OutlineSelection then Scene.PostEffects.RemoveIndex(OutlineFX);
  SwapBuffers();
end;

function TVoxView.MeasureFPS(Interval: Integer = 500; MinSamples: Integer = 2): Double;
var
  Accumulator: Double = 0.0;
  I: Integer;
begin
  for I := 0 to Length(FrameTimestamps) - 2 do begin
    if (I >= MinSamples) and ((FrameTimestamps[0] - FrameTimestamps[I + 1]) > Interval) then break;
    Accumulator += Sqr(0.001 * (FrameTimestamps[I] - FrameTimestamps[I + 1]));
  end;
  Result := Sqrt(I / Accumulator);
end;

procedure TVoxView.SelectMeshAt(X, Y: Integer);
begin
  if SelectedMesh <> Nil then begin
    Scene.MetaRenderer.Draw(Nil, [SelectedMesh]); // Something is already selected -> ignore the selected mesh
  end else Scene.MetaRenderer.Draw(Nil);
  SelectedMesh := Scene.MetaRenderer.MeshAt(X, Y);
end;

procedure TVoxView.FitVisibleToScreen(Margin: Double = 0.0);
var
  MinNDC, MaxNDC, CenterNDC: TVec2;
  MinZ: Double;
  Mesh: TVoxMesh;
  Vertex: TVertex;
  ProjectedVertex, CenteringAdjustment, DepthAdjustment: TVec3;
begin
  if Scene.Meshes.Count = 0 then Exit();
  Scene.Camera.Projection := prOrthographic;
  MinNDC := Vec2(Infinity, Infinity);
  MaxNDC := Vec2(-Infinity, -Infinity);
  MinZ := Infinity;
  for Mesh in Scene.Meshes do begin
    for Vertex in Mesh.Vertices do begin
      ProjectedVertex := Scene.Camera.Project(Vertex.Position);
      MinNDC.X := Min(MinNDC.X, ProjectedVertex.X);
      MinNDC.Y := Min(MinNDC.Y, ProjectedVertex.Y);
      MaxNDC.X := Max(MaxNDC.X, ProjectedVertex.X);
      MaxNDC.Y := Max(MaxNDC.Y, ProjectedVertex.Y);
      MinZ := Min(MinZ, ProjectedVertex.Z);
    end;
  end;
  with Scene.Camera do begin
    Pivot := Project(Vec3(0.0, 0.0, MinZ), True);
    CenterNDC := (MinNDC + MaxNDC) / 2.0;
    CenteringAdjustment := Project(Vec3(CenterNDC, 0.0), True) - Project(Vec3(0.0, 0.0, 0.0), True);
    DepthAdjustment := Pivot - Position - Direction * Log10((NearPlane + FarPlane) / 2.0);
    Position += CenteringAdjustment + DepthAdjustment;
    OrthoSize *= Max(MaxNDC.X - MinNDC.X, MaxNDC.Y - MinNDC.Y) / 2.0 * (Margin + 1.0);
  end;
end;

procedure TVoxView.SetProjection(Projection: TVoxProjection);
var
  OldPivotDepth, NewPivotDepth: Double;
begin
  OldPivotDepth := Scene.Camera.Direction.Dot(Pivot - Scene.Camera.Position);
  if (Scene.Camera.Projection = prPerspective) and (Projection = prOrthographic) then begin
    Scene.Camera.OrthoSize := 2.0 * Tan(Scene.Camera.FOV / 2.0) * OldPivotDepth;
  end else if (Scene.Camera.Projection = prOrthographic) and (Projection = prPerspective) then begin
    NewPivotDepth := Scene.Camera.OrthoSize / (2.0 * Tan(Scene.Camera.FOV / 2.0));
    Scene.Camera.Position += Scene.Camera.Direction * (OldPivotDepth - NewPivotDepth);
  end;
  Scene.Camera.Projection := Projection;
end;

procedure TVoxView.ViewSide(Side: TVoxViewSide; Margin: Double = 0.0);
var
  Direction: TVec3;
begin
  Direction := Vec3(-0.5773503, -0.5773503, 0.5773503); // Isometric from top-right
  SetProjection(prOrthographic);
  case Side of
    vsRight:  Direction := Vec3(-1.0, 0.0, 0.0);
    vsLeft:   Direction := Vec3(1.0, 0.0, 0.0);
    vsTop:    Direction := Vec3(0.0, -1.0, 1e-5);
    vsBottom: Direction := Vec3(0.0, 1.0, 1e-5);
    vsBack:   Direction := Vec3(0.0, 0.0, -1.0);
    vsFront:  Direction := Vec3(0.0, 0.0, 1.0);
  end;
  Scene.Camera.Direction := (Scene.Camera.ModelMatrix().Inverse() * Direction).Normal();
  FitVisibleToScreen(Margin);
end;

procedure TVoxView.NavigateOrbit(DX, DY: Double);
var
  YawDelta, PitchDelta: Double;
  PureRotationMatrix, FlatProjectionMatrix: TMatrix4;
  PrevDirection: TVec3;
begin
  YawDelta := -0.01 * DX * OrbitSpeed / 100.0 * IfThen(IsOrbitTranspolar, -1.0, 1.0);
  PitchDelta := -0.01 * DY * OrbitSpeed / 100.0;
  with Scene.Camera do begin
    PrevDirection := Direction;
    PureRotationMatrix := TransformationMatrix([
      RotationMatrix(RightVector(), PitchDelta),
      RotationMatrix(VisualWorldUp, YawDelta)
    ]);
    Direction := PureRotationMatrix * Direction;
    Position := TransformationMatrix([
      TranslationMatrix(-Pivot),
      PureRotationMatrix,
      TranslationMatrix(Pivot)
    ]) * Position;
    FlatProjectionMatrix := FlatteningMatrix(VisualWorldUp);
    if Vec2(FlatProjectionMatrix * Direction).Dot(Vec2(FlatProjectionMatrix * PrevDirection)) < 0.0 then begin
      IsOrbitTranspolar := not IsOrbitTranspolar;
      VisualWorldUp := -VisualWorldUp;
    end;
  end;
end;

procedure TVoxView.NavigateZoom(DW: Double);
var
  ScaleDelta: Double;
begin
  ScaleDelta := -0.2 * DW / 120.0 * ZoomSpeed / 100.0;
  if DW < 0.0 then ScaleDelta /= 1.0 - ScaleDelta;
  Scene.Camera.Position := (Scene.Camera.Position - Pivot) * (1.0 + ScaleDelta) + Pivot;
  Scene.Camera.OrthoSize *= (1.0 + ScaleDelta);
end;

procedure TVoxView.NavigateDolly(DY: Double);
var
  DollyVector: TVec3;
  OrthoPivotDist: Double;
begin
  DollyVector := Scene.Camera.Position - Pivot;
  OrthoPivotDist := DollyVector.Dot(Scene.Camera.Direction);
  Scene.Camera.Position += DollyVector.Normal() * (-0.01 * OrthoPivotDist * DY * DollySpeed / 100.0);
end;

procedure TVoxView.NavigatePan(DX, DY: Double);
var
  PannedPivot, DeltaNormalized: TVec3;
begin
  DeltaNormalized := Vec3(-DX / Scene.Width, DY / Scene.Height, 0.0) * 2.0 * PanSpeed / 100.0;
  PannedPivot := Scene.Camera.Project(Scene.Camera.Project(Pivot) + DeltaNormalized, True);
  Scene.Camera.Position += PannedPivot - Pivot;
end;

{$endif}
