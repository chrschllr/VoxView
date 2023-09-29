{interface}
{$ifdef INCLUDE_SECTION_INTERFACE}

type
  TVoxAttachmentInfo = record
    Format: TVoxTexForm;
    AttachmentPoint: GLuint;
  end;
  TVoxAttachedTexture = record
    Texture: TVoxTexture;
    AttachmentPoint: GLuint;
  end;
  TVoxFramebuffer = class(TStoredErrorObject)
  private
    FWidth, FHeight: Integer;
    FSamples: Integer;
    Attachments: array of TVoxAttachedTexture;
    FColorAttachments: array of TVoxAttachedTexture;
    FDepthAttachment, FStencilAttachment: TVoxAttachedTexture;
    function GetColorAttachment(const I: Integer): TVoxTexture;
  public
    FBO: GLuint;
    constructor Create(AWidth, AHeight: Integer; const AAttachmentInfos: array of TVoxAttachmentInfo;
                       ASamples: Integer = 1);
    destructor Destroy(); override;
    procedure Resize(NewWidth, NewHeight: Integer);
    procedure CopyTo(const Destination: TVoxFramebuffer;
                     SrcColorAttachmentN, DstColorAttachmentN: array of Integer; Depth: Boolean = True);
    procedure Bind(const ColorAttribDrawIndices: array of Integer; AdjustViewport: Boolean = True);
    function ReadAsBitmap(Buffer: GLenum = GL_COLOR_ATTACHMENT0): TBitmap;
    property Width: Integer read FWidth;
    property Height: Integer read FHeight;
    property Samples: Integer read FSamples;
    property ColorAttachments[const I: Integer]: TVoxTexture read GetColorAttachment; default;
    property DepthAttachment: TVoxAttachedTexture read FDepthAttachment;
    property StencilAttachment: TVoxAttachedTexture read FStencilAttachment;
  end;

function AttachmentInfo(AttachmentPoint: GLenum; const Format: TVoxTexForm; Target: GLenum = 0): TVoxAttachmentInfo;

{$endif}

{implementation}
{$ifdef INCLUDE_SECTION_IMPLEMENTATION}

function AttachmentInfo(AttachmentPoint: GLenum; const Format: TVoxTexForm; Target: GLenum = 0): TVoxAttachmentInfo;
begin
  Result.Format := Format;
  if Target <> 0 then Result.Format.Target := Target;
  Result.AttachmentPoint := AttachmentPoint;
end;

// #########################################################
// ###                 TVoxFramebuffer                   ###
// #########################################################

function TVoxFramebuffer.GetColorAttachment(const I: Integer): TVoxTexture;
begin
  Result := FColorAttachments[I].Texture;
end;

constructor TVoxFramebuffer.Create(AWidth, AHeight: Integer; const AAttachmentInfos: array of TVoxAttachmentInfo;
                                   ASamples: Integer = 1);
var
  I: Integer;
  NewTexture: GLuint;
  ColorAttachmentCount: Integer = 0;
begin
  FWidth := AWidth;
  FHeight := AHeight;
  FSamples := ASamples;
  // Framebuffer configuration
  glGenFramebuffers(1, @FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);
  // Attachment textures
  SetLength(Attachments, Length(AAttachmentInfos));
  SetLength(FColorAttachments, Length(AAttachmentInfos)); // Worst-case, properly resized later on
  for I := 0 to Length(Attachments) - 1 do begin
    with AAttachmentInfos[I] do begin
      glGenTextures(1, @NewTexture);
      glBindTexture(Format.Target, NewTexture);
      glTexParameteri(Format.Target, GL_TEXTURE_MIN_FILTER, Format.MinFilter);
      glTexParameteri(Format.Target, GL_TEXTURE_MAG_FILTER, Format.MagFilter);
      glTexParameteri(Format.Target, GL_TEXTURE_WRAP_S, Format.WrapHorizontal);
      glTexParameteri(Format.Target, GL_TEXTURE_WRAP_T, Format.WrapVertical);
      if (AttachmentPoint = GL_DEPTH_ATTACHMENT) or (AttachmentPoint = GL_STENCIL_ATTACHMENT) or
         (AttachmentPoint = GL_DEPTH_STENCIL_ATTACHMENT) then begin
        glTexParameteri(Format.Target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glTexParameteri(Format.Target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        case AttachmentPoint of
          GL_DEPTH_ATTACHMENT: begin
            FDepthAttachment.AttachmentPoint := AttachmentPoint;
            FDepthAttachment.Texture.Name := NewTexture;
            FDepthAttachment.Texture.Format := Format;
          end;
          GL_STENCIL_ATTACHMENT: begin
            FStencilAttachment.AttachmentPoint := AttachmentPoint;
            FStencilAttachment.Texture.Name := NewTexture;
            FStencilAttachment.Texture.Format := Format;
          end;
          GL_DEPTH_STENCIL_ATTACHMENT: begin
            FDepthAttachment.AttachmentPoint := AttachmentPoint;
            FDepthAttachment.Texture.Name := NewTexture;
            FDepthAttachment.Texture.Format := Format;
            FStencilAttachment := FDepthAttachment;
          end;
        end;
      end else begin
        FColorAttachments[ColorAttachmentCount].AttachmentPoint := AttachmentPoint;
        FColorAttachments[ColorAttachmentCount].Texture.Name := NewTexture;
        FColorAttachments[ColorAttachmentCount].Texture.Format := Format;
        ColorAttachmentCount += 1;
      end;
      glFramebufferTexture2D(GL_FRAMEBUFFER, AttachmentPoint, Format.Target, NewTexture, 0);
      Attachments[I].AttachmentPoint := AttachmentPoint;
      Attachments[I].Texture.Name := NewTexture;
      Attachments[I].Texture.Format := Format;
    end;
  end;
  SetLength(FColorAttachments, ColorAttachmentCount);
  // Allocate attachment textures
  Resize(Width, Height);
  I := glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if I <> GL_FRAMEBUFFER_COMPLETE then Error := Format('Framebuffer creation error (%d)', [I]);
end;

destructor TVoxFramebuffer.Destroy();
var
  I: Integer;
begin
  for I := 0 to Length(Attachments) - 1 do glDeleteTextures(1, @Attachments[I].Texture.Name);
  glDeleteFramebuffers(1, @FBO);
  inherited Destroy();
end;

procedure TVoxFramebuffer.Resize(NewWidth, NewHeight: Integer);
var
  I: Integer;
begin
  FWidth := NewWidth;
  FHeight := NewHeight;
  for I := 0 to Length(Attachments) - 1 do with Attachments[I].Texture.Format do begin
    glBindTexture(Target, Attachments[I].Texture.Name);
    if Target = GL_TEXTURE_2D_MULTISAMPLE then begin
      glTexImage2DMultisample(Target, Samples, InternalFormat, Width, Height, GL_TRUE);
    end else begin
      glTexImage2D(Target, 0, InternalFormat, Width, Height, 0, PixelFormat, PixelType, PGLvoid(0));
    end;
  end;
end;

procedure TVoxFramebuffer.CopyTo(const Destination: TVoxFramebuffer;
                                 SrcColorAttachmentN, DstColorAttachmentN: array of Integer; Depth: Boolean = True);
var
  SpecialAttachment: TVoxAttachedTexture;
  I: Integer;
begin
  glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Destination.FBO);
  if Depth then begin
    SpecialAttachment := DepthAttachment;
    if SpecialAttachment.Texture.Name <> GL_INVALID_VALUE then begin
      glBlitFramebuffer(
        0, 0, Width, Height,
        0, 0, Destination.Width, Destination.Height,
        GL_DEPTH_BUFFER_BIT, GL_NEAREST
      );
    end;
  end;
  for I := 0 to Min(Length(SrcColorAttachmentN), Length(DstColorAttachmentN)) - 1 do begin
    glReadBuffer(GL_COLOR_ATTACHMENT0 + SrcColorAttachmentN[I]);
    glDrawBuffer(GL_COLOR_ATTACHMENT0 + DstColorAttachmentN[I]);
    glBlitFramebuffer(
      0, 0, Width, Height,
      0, 0, Destination.Width, Destination.Height,
      GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
  end;
end;

procedure TVoxFramebuffer.Bind(const ColorAttribDrawIndices: array of Integer; AdjustViewport: Boolean = True);
var
  ArgDrawBuffers: array of GLenum;
  I: Integer;
begin
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);
  if AdjustViewport then glViewport(0, 0, Width, Height);
  SetLength(ArgDrawBuffers{%H-}, Length(ColorAttribDrawIndices));
  for I := 0 to Length(ArgDrawBuffers) - 1 do begin
    ArgDrawBuffers[I] := GL_COLOR_ATTACHMENT0 + ColorAttribDrawIndices[I];
  end;
  glDrawBuffers(Length(ArgDrawBuffers), @ArgDrawBuffers[0]);
end;

function TVoxFramebuffer.ReadAsBitmap(Buffer: GLenum = GL_COLOR_ATTACHMENT0): TBitmap;
begin
  Result := TBitmap.Create();
  Result.PixelFormat := pf32bit;
  Result.Width := Width;
  Result.Height := Height;
  glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
  glReadBuffer(Buffer);
  glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, PGLvoid(Result.RawImage.Data));
end;

{$endif}
