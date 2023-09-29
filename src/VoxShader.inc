{interface}
{$ifdef INCLUDE_SECTION_INTERFACE}

{$r Shaders.rc}

type
  TGLuintArray = array of GLuint;
  TGLenumArray = array of GLenum;
  TStoredErrorObject = class
  private
    FError: String;
    procedure SetError(AError: String);
  public
    function PopError(): String;
    property Error: String read FError write SetError;
  end;
  TVoxPhongMaterial = record
    Ambient, Diffuse, Specular: TColor4;
    Shininess: GLfloat; // Higher values mean smaller specular hot spots
    Alpha: GLfloat;
    class operator =(const MatA, MatB: TVoxPhongMaterial): Boolean;
    function ToString(): String;
  end;
  TVoxPhongLampMode = (lmSun = 0, lmPoint = 1);
  TVoxPhongLamp = record
    Mode: TVoxPhongLampMode;
    Position: TVec3; // Acts as incident vector when Mode is lmSun
    Ambient, Diffuse, Specular: TColor4; // Colors/intensities for the 3 components
    Constant, Linear, Quadratic: GLfloat; // Attenuation function parameters for point lamps
    class operator =(const LampA, LampB: TVoxPhongLamp): Boolean;
    function ToString(): String;
  end;
  TVoxPhongLampList = specialize TArrayList<TVoxPhongLamp>;
  TVoxUniformMap = class;
  TVoxShader = class(TStoredErrorObject)
  private
    class function GetCompilerError(Shader: GLuint; out Message: String): Boolean; static;
    class function GetLinkerError(ShaderProgram: GLuint; out Message: String): Boolean; static;
    function GetUniformLocation(const Identifier: String): GLint;
  public
    ShaderProgram: GLuint;
    Uniforms: TVoxUniformMap;
    IsValid: Boolean;
    class function Compile(ShaderType: GLenum; const Source: String; out CompilerError: String): GLuint; static;
    class function Link(const ShaderObjects: TGLuintArray; out LinkerError: String): GLuint; static;
    constructor Create();
    constructor Create(const VertexShaderSource: String; const FragmentShaderSource: String);
    destructor Destroy(); override;
    function Build(const ShaderSources: TStringArray; const ShaderTypes: TGLenumArray = Nil): Boolean;
    property UniformLocation[const Identifier: String]: GLint read GetUniformLocation; default;
  end;
  TVoxUniformMap = class
  private
    Keys: array of String;
    Values: array of GLint;
  public
    Shader: TVoxShader;
    constructor Create(const AShader: TVoxShader);
    procedure Clear();
    function GetLocation(const Identifier: String): GLint;
    property Location[const Identifier: String]: GLint read GetLocation; default;
  end;

const
  PHONG_MATERIAL_DEFAULT: TVoxPhongMaterial = (
    Ambient:  (R: 0.8; G: 0.8; B: 0.8; A: NaN);
    Diffuse:  (R: 1.0; G: 1.0; B: 1.0; A: NaN);
    Specular: (R: 1.0; G: 1.0; B: 1.0; A: NaN);
    Shininess: 32.0;
    Alpha: 1.0;
  );
  PHONG_LAMP_SUN_DEFAULT: TVoxPhongLamp = (
    Mode: lmSun;
    Position: (X: -0.371; Y: 0.557; Z: -0.743);
    Ambient:  (R: 0.200; G: 0.200; B: 0.200; A: NaN);
    Diffuse:  (R: 0.800; G: 0.800; B: 0.800; A: NaN);
    Specular: (R: 0.500; G: 0.500; B: 0.500; A: NaN);
    Constant: 1.0; Linear: 0.05; Quadratic: 0.008;
  );
  PHONG_LAMP_4P_WHITE_DEFAULT: TVoxPhongLamp = (
    Mode: lmPoint;
    Position: (X: -0.590; Y: 0.643; Z: 0.487);
    Ambient:  (R: 0.059; G: 0.059; B: 0.059; A: NaN);
    Diffuse:  (R: 0.588; G: 0.588; B: 0.588; A: NaN);
    Specular: (R: 0.361; G: 0.361; B: 0.361; A: NaN);
    Constant: 1.0; Linear: 0.05; Quadratic: 0.008;
  );
  PHONG_LAMP_4P_ORANGE_DEFAULT: TVoxPhongLamp = (
    Mode: lmPoint;
    Position: (X: 0.763; Y: 0.251; Z: -0.595);
    Ambient:  (R: 0.000; G: 0.000; B: 0.000; A: NaN);
    Diffuse:  (R: 0.420; G: 0.353; B: 0.294; A: NaN);
    Specular: (R: 0.490; G: 0.278; B: 0.082; A: NaN);
    Constant: 1.0; Linear: 0.05; Quadratic: 0.008;
  );
  PHONG_LAMP_4P_BLUE_DEFAULT: TVoxPhongLamp = (
    Mode: lmPoint;
    Position: (X: -0.412; Y: -0.640; Z: -0.648);
    Ambient:  (R: 0.000; G: 0.000; B: 0.000; A: NaN);
    Diffuse:  (R: 0.525; G: 0.573; B: 0.690; A: NaN);
    Specular: (R: 0.094; G: 0.208; B: 0.588; A: NaN);
    Constant: 1.0; Linear: 0.05; Quadratic: 0.008;
  );
  PHONG_LAMP_4P_RED_DEFAULT: TVoxPhongLamp = (
    Mode: lmPoint;
    Position: (X: -0.080; Y: -0.478; Z: 0.875);
    Ambient:  (R: 0.000; G: 0.000; B: 0.000; A: NaN);
    Diffuse:  (R: 0.424; G: 0.290; B: 0.294; A: NaN);
    Specular: (R: 0.533; G: 0.235; B: 0.235; A: NaN);
    Constant: 1.0; Linear: 0.05; Quadratic: 0.008;
  );

var
  SHADER_VERT_MODEL,                                                        // Model (projection)
  SHADER_FRAG_GRID,                                                         // Scene/world background grid
  SHADER_VERT_SCREEN, SHADER_FRAG_SCREEN,                                   // Fullscreen quad (+basic post-processing)
  SHADER_FRAG_FXAA,                                                         // FXAA (Post-processing antialiasing)
  SHADER_FRAG_OUTLINE,                                                      // Object outline post-processing
  SHADER_FRAG_DEFAULT_BASIC,                                                // Default renderer (normals as color)
  SHADER_FRAG_DEFAULT,                                                      // Default renderer
  SHADER_FRAG_DDP_PEEL, SHADER_FRAG_DDP_BACKGROUND,                         // Dual-depth-peeling renderer...
  SHADER_FRAG_DDP_BACKBLEND, SHADER_FRAG_DDP_FINAL,                         // ...for transparency support
  SHADER_FRAG_META: String;                                                 // Meta renderer

{$endif}

{implementation}
{$ifdef INCLUDE_SECTION_IMPLEMENTATION}

var
  LogStoredErrors: Boolean = False;
  LogFile: TextFile; // Initialized in package main unit

function LoadTextResource(ResourceName: String): String;
var
  StreamIn: TResourceStream;
  StreamOut: TStringStream;
begin
  Result := '';
  StreamIn := TResourceStream.Create(HINSTANCE(), ResourceName, RT_RCDATA);
  StreamOut := TStringStream.Create('');
  try
    StreamOut.CopyFrom(StreamIn, StreamIn.Size);
    Result := StreamOut.DataString;
  finally
    StreamIn.Free();
    StreamOut.Free();
  end;
end;

// #########################################################
// ###                 TVoxPhongMaterial                 ###
// #########################################################

class operator TVoxPhongMaterial.=(const MatA, MatB: TVoxPhongMaterial): Boolean;
begin
  Result := CompareByte(MatA, MatB, SizeOf(MatA)) = 0;
end;

function TVoxPhongMaterial.ToString(): String;
begin
  Result := Format('TVoxPhongMaterial {', []) + LineEnding;
  Result += Format('  Ambient: %s,', [Ambient.ToString()]) + LineEnding;
  Result += Format('  Diffuse: %s,', [Diffuse.ToString()]) + LineEnding;
  Result += Format('  Specular: %s,', [Specular.ToString()]) + LineEnding;
  Result += Format('  Shininess: %.3f,', [Shininess], VOX_FORMAT) + LineEnding;
  Result += Format('  Alpha: %.3f', [Alpha], VOX_FORMAT) + LineEnding;
  Result += Format('}', []);
end;

// #########################################################
// ###                   TVoxPhongLamp                   ###
// #########################################################

class operator TVoxPhongLamp.=(const LampA, LampB: TVoxPhongLamp): Boolean;
begin
  Result := CompareByte(LampA, LampB, SizeOf(LampA)) = 0;
end;

function TVoxPhongLamp.ToString(): String;
var
  ModeStr: String;
begin
  WriteStr(ModeStr, Mode);
  Result := Format('TVoxPhongLamp {', []) + LineEnding;
  Result += Format('  Mode: %s,', [ModeStr]) + LineEnding;
  Result += Format('  Ambient: %s,', [Ambient.ToString()]) + LineEnding;
  Result += Format('  Diffuse: %s,', [Diffuse.ToString()]) + LineEnding;
  Result += Format('  Specular: %s,', [Specular.ToString()]) + LineEnding;
  Result += Format('  Constant: %.3f,', [Constant], VOX_FORMAT) + LineEnding;
  Result += Format('  Linear: %.3f,', [Linear], VOX_FORMAT) + LineEnding;
  Result += Format('  Quadratic: %.3f', [Quadratic], VOX_FORMAT) + LineEnding;
  Result += Format('}', []);
end;

// #########################################################
// ###                TStoredErrorObject                 ###
// #########################################################

procedure TStoredErrorObject.SetError(AError: String);
begin
  FError := AError;
  if LogStoredErrors and (FError <> '') then begin
    Append(LogFile);
    WriteLn(LogFile, Format('%s in %s:', [DateTimeToStr(Now()), ClassName]));
    WriteLn(LogFile, FError);
    Close(LogFile);
  end;
end;

function TStoredErrorObject.PopError(): String;
begin
  Result := FError;
  FError := '';
end;

// #########################################################
// ###                     TVoxShader                    ###
// #########################################################

class function TVoxShader.GetCompilerError(Shader: GLuint; out Message: String): Boolean;
var
  Status, ErrorLogLength: GLint;
begin
  Message := '';
  glGetShaderiv(Shader, GL_COMPILE_STATUS, @Status);
  Result := (Status = 0); // Status 0 -> OK
  if not Result then Exit();
  glGetError();
  glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, @ErrorLogLength);
  SetLength(Message, ErrorLogLength);
  glGetShaderInfoLog(Shader, ErrorLogLength, @ErrorLogLength, PGLchar(Message));
  Message := Message.Trim();
end;

class function TVoxShader.GetLinkerError(ShaderProgram: GLuint; out Message: String): Boolean;
var
  Status, ErrorLogLength: GLint;
begin
  Message := '';
  glGetProgramiv(ShaderProgram, GL_LINK_STATUS, @Status);
  Result := (Status = 0); // Status 0 -> OK
  if not Result then Exit();
  glGetError();
  glGetProgramiv(ShaderProgram, GL_INFO_LOG_LENGTH, @ErrorLogLength);
  SetLength(Message, ErrorLogLength);
  glGetProgramInfoLog(ShaderProgram, ErrorLogLength, @ErrorLogLength, PGLchar(Message));
  Message := Message.Trim();
end;

function TVoxShader.GetUniformLocation(const Identifier: String): GLint;
begin
  Result := Uniforms[Identifier];
end;

class function TVoxShader.Compile(ShaderType: GLenum; const Source: String; out CompilerError: String): GLuint;
begin
  Result := glCreateShader(ShaderType);
  glShaderSource(Result, 1, PGLchar(@Source), Nil);
  glCompileShader(Result);
  if GetCompilerError(Result, CompilerError) then begin
    glDeleteShader(Result);
    Result := 0;
  end;
end;

class function TVoxShader.Link(const ShaderObjects: TGLuintArray; out LinkerError: String): GLuint;
var
  Shader: GLuint;
begin
  Result := glCreateProgram();
  for Shader in ShaderObjects do glAttachShader(Result, Shader);
  glLinkProgram(Result);
  if GetLinkerError(Result, LinkerError) then Result := 0;
  for Shader in ShaderObjects do glDeleteShader(Shader);
end;

constructor TVoxShader.Create();
begin
  Uniforms := TVoxUniformMap.Create(Self);
end;

constructor TVoxShader.Create(const VertexShaderSource: String; const FragmentShaderSource: String);
begin
  Create();
  Build([VertexShaderSource, FragmentShaderSource], [GL_VERTEX_SHADER, GL_FRAGMENT_SHADER]);
end;

destructor TVoxShader.Destroy();
begin
  glDeleteProgram(ShaderProgram);
  Uniforms.Free();
  inherited Destroy();
end;

function TVoxShader.Build(const ShaderSources: TStringArray; const ShaderTypes: TGLenumArray = Nil): Boolean;
var
  ShaderObjects: TGLuintArray;
  ShaderType: GLenum;
  ShaderBinary: GLuint;
  BuildError: String;
  ErrorSource: String = '';
  I: Integer;
begin
  IsValid := False; // TODO: Investigate segfault when returning a vector of the wrong size in GLSL
  ShaderObjects := Default(TGLuintArray);
  SetLength(ShaderObjects, Length(ShaderSources));
  for I := 0 to Length(ShaderSources) - 1 do begin
    ShaderType := IfThen(I < Length(ShaderTypes), ShaderTypes[I], GL_INVALID_ENUM);
    ShaderObjects[I] := TVoxShader.Compile(ShaderType, ShaderSources[I], BuildError);
    if ShaderObjects[I] = 0 then case ShaderType of
      GL_VERTEX_SHADER: ErrorSource := 'vertex shader';
      GL_FRAGMENT_SHADER: ErrorSource := 'fragment shader';
      GL_GEOMETRY_SHADER: ErrorSource := 'geometry shader';
      GL_TESS_CONTROL_SHADER: ErrorSource := 'tessellation control shader';
      GL_TESS_EVALUATION_SHADER: ErrorSource := 'tessellation evaluation shader';
    end;
    if ErrorSource <> '' then begin
      Error := BuildError + LineEnding + Format('(while compiling %s)', [ErrorSource]);
      Exit(False);
    end;
  end;
  ShaderBinary := TVoxShader.Link(ShaderObjects, BuildError);
  if ShaderBinary <> 0 then begin
    if ShaderProgram <> 0 then glDeleteProgram(ShaderProgram);
    ShaderProgram := ShaderBinary;
    Uniforms.Clear();
    IsValid := True;
    Exit(True);
  end else Error := BuildError + LineEnding + '(while linking program)';
  Result := False;
end;

// #########################################################
// ###                  TVoxUniformMap                   ###
// #########################################################

constructor TVoxUniformMap.Create(const AShader: TVoxShader);
begin
  Shader := AShader;
end;

procedure TVoxUniformMap.Clear();
begin
  SetLength(Keys, 0);
  SetLength(Values, 0);
end;

function TVoxUniformMap.GetLocation(const Identifier: String): GLint;
var
  I: Integer;
begin
  for I := 0 to Length(Keys) - 1 do begin
    if Keys[I] = Identifier then Exit(Values[I]);
  end;
  Result := glGetUniformLocation(Shader.ShaderProgram, PGLchar(Identifier));
  SetLength(Keys, Length(Keys) + 1);
  SetLength(Values, Length(Values) + 1);
  Keys[Length(Keys) - 1] := Identifier;
  Values[Length(Values) - 1] := Result;
end;

{$endif}

{initialization}
{$ifdef INCLUDE_SECTION_INITIALIZATION}

begin
  SHADER_VERT_MODEL := LoadTextResource('SHADER_VERT_MODEL');
  SHADER_FRAG_GRID := LoadTextResource('SHADER_FRAG_GRID');
  SHADER_VERT_SCREEN := LoadTextResource('SHADER_VERT_SCREEN');
  SHADER_FRAG_SCREEN := LoadTextResource('SHADER_FRAG_SCREEN');
  SHADER_FRAG_FXAA := LoadTextResource('SHADER_FRAG_FXAA');
  SHADER_FRAG_OUTLINE := LoadTextResource('SHADER_FRAG_OUTLINE');
  SHADER_FRAG_DEFAULT_BASIC := LoadTextResource('SHADER_FRAG_DEFAULT_BASIC');
  SHADER_FRAG_DEFAULT := LoadTextResource('SHADER_FRAG_DEFAULT');
  SHADER_FRAG_DDP_BACKGROUND := LoadTextResource('SHADER_FRAG_DDP_BACKGROUND');
  SHADER_FRAG_DDP_PEEL := LoadTextResource('SHADER_FRAG_DDP_PEEL');
  SHADER_FRAG_DDP_BACKBLEND := LoadTextResource('SHADER_FRAG_DDP_BACKBLEND');
  SHADER_FRAG_DDP_FINAL := LoadTextResource('SHADER_FRAG_DDP_FINAL');
  SHADER_FRAG_META := LoadTextResource('SHADER_FRAG_META');
end;

{$endif}
