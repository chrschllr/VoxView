{interface}
{$ifdef INCLUDE_SECTION_INTERFACE}

type
  TVoxTexForm = record
    Target: GLenum;
    InternalFormat: GLenum;
    PixelFormat, PixelType: Glenum;
    MinFilter, MagFilter: GLint;
    WrapHorizontal, WrapVertical: GLint;
  end;
  TVoxTexture = record
    Name: GLuint;
    Format: TVoxTexForm;
    class operator =(const TexA, TexB: TVoxTexture): Boolean;
    procedure UploadToGPU(const Image: TBGRABitmap);
    function DownloadFromGPU(): TBGRABitmap;
  end;

const
  GL_R32I = $8235;
  GL_R32F = $822E;
  GL_RG32F = $8230;
  GL_RG = $8227;
  TEX_FORM_COLOR: TVoxTexForm = (
    Target: GL_TEXTURE_2D;
    InternalFormat: GL_RGBA;
    PixelFormat: {$ifdef Windows} GL_BGRA {$else} GL_RGBA {$endif}; PixelType: GL_UNSIGNED_BYTE;
    MinFilter: GL_LINEAR; MagFilter: GL_LINEAR;
    WrapHorizontal: GL_CLAMP_TO_EDGE; WrapVertical: GL_CLAMP_TO_EDGE
  );
  TEX_FORM_DEPTH: TVoxTexForm = (
    Target: GL_TEXTURE_2D;
    InternalFormat: GL_DEPTH_COMPONENT32; // 32-bit depth buffer
    PixelFormat: GL_DEPTH_COMPONENT; PixelType: GL_FLOAT;
    MinFilter: GL_LINEAR; MagFilter: GL_LINEAR;
    WrapHorizontal: GL_CLAMP_TO_EDGE; WrapVertical: GL_CLAMP_TO_EDGE
  );
  TEX_FORM_INDEX: TVoxTexForm = (
    Target: GL_TEXTURE_2D;
    InternalFormat: GL_R32I;
    PixelFormat: GL_RED_INTEGER; PixelType: GL_INT;
    MinFilter: GL_NEAREST; MagFilter: GL_NEAREST;
    WrapHorizontal: GL_CLAMP_TO_EDGE; WrapVertical: GL_CLAMP_TO_EDGE
  );
  TEX_FORM_FLOAT: TVoxTexForm = (
    Target: GL_TEXTURE_2D;
    InternalFormat: GL_R32F;
    PixelFormat: GL_RED; PixelType: GL_FLOAT;
    MinFilter: GL_NEAREST; MagFilter: GL_NEAREST;
    WrapHorizontal: GL_CLAMP_TO_EDGE; WrapVertical: GL_CLAMP_TO_EDGE
  );
  TEX_FORM_DUAL_DEPTH: TVoxTexForm = (
    Target: GL_TEXTURE_2D;
    InternalFormat: GL_RG32F;
    PixelFormat: GL_RG; PixelType: GL_FLOAT;
    MinFilter: GL_NEAREST; MagFilter: GL_NEAREST;
    WrapHorizontal: GL_CLAMP_TO_EDGE; WrapVertical: GL_CLAMP_TO_EDGE
  );
  TEX_FORM_POSITION: TVoxTexForm = (
    Target: GL_TEXTURE_2D;
    InternalFormat: GL_RGB32F;
    PixelFormat: GL_RGB; PixelType: GL_FLOAT;
    MinFilter: GL_NEAREST; MagFilter: GL_NEAREST;
    WrapHorizontal: GL_CLAMP_TO_EDGE; WrapVertical: GL_CLAMP_TO_EDGE
  );

{$endif}

{implementation}
{$ifdef INCLUDE_SECTION_IMPLEMENTATION}

// #########################################################
// ###                   TVoxTexture                     ###
// #########################################################

class operator TVoxTexture.=(const TexA, TexB: TVoxTexture): Boolean;
begin
  Result := (TexA.Name = TexB.Name);
end;

procedure TVoxTexture.UploadToGPU(const Image: TBGRABitmap);
var
  AdjustedImage: TBGRABitmap;
begin
  // Generate GPU-side texture
  glGenTextures(1, @Name); // When the package has debug info enabled, this seems to fail randomly...
  glBindTexture(Format.Target, Name); // I have no idea why, but at least it won't happen in production
  // Texture parameters: linear filtering, black outside borders
  glTexParameteri(Format.Target, GL_TEXTURE_MIN_FILTER, Format.MinFilter);
  glTexParameteri(Format.Target, GL_TEXTURE_MAG_FILTER, Format.MagFilter);
  glTexParameteri(Format.Target, GL_TEXTURE_WRAP_S, Format.WrapHorizontal);
  glTexParameteri(Format.Target, GL_TEXTURE_WRAP_T, Format.WrapVertical);
  // Upload image data
  {$ifdef Windows}
  AdjustedImage := Image;
  {$else}
  AdjustedImage := Image.Duplicate();
  AdjustedImage.VerticalFlip();
  {$endif}
  glTexImage2D(
    Format.Target,                  // Target (texture type)
    0,                              // Level (used for mipmapping)
    Format.InternalFormat,          // Internal format (e.g. GL_RGBA, GL_DEPTH_STENCIL)
    Image.Width,                    // Width
    Image.Height,                   // Height
    0,                              // Border (must be 0)
    Format.PixelFormat,             // Pixel format (of following pixel data)
    Format.PixelType,               // Pixel type (of following pixel data)
    PGLvoid(AdjustedImage.Data)     // Pixel data
  );
  {$ifndef Windows}
  AdjustedImage.Free();
  {$endif}
  glGenerateMipmap(GL_TEXTURE_2D);  // Mipmap generation
end;

function TVoxTexture.DownloadFromGPU(): TBGRABitmap;
var
  Width: Integer = 0;
  Height: Integer = 0;
begin
  glBindTexture(Format.Target, Name);
  glGetTexLevelParameteriv(Format.Target, 0, GL_TEXTURE_WIDTH, @Width);
  glGetTexLevelParameteriv(Format.Target, 0, GL_TEXTURE_HEIGHT, @Height);
  Result := TBGRABitmap.Create(Width, Height);
  if (Width = 0) or (Height = 0) then Exit();
  glGetTexImage(Format.Target, 0, Format.PixelFormat, Format.PixelType, Result.Data);
end;



{$endif}
