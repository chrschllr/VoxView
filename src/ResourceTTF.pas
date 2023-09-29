unit ResourceTTF;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Resource,
  LazFreeType, TTTypes,
  BGRABitmapTypes, BGRABitmap, BGRAVectorize,
  {$ifdef Windows} Windows {$else} MD5 {$endif};

type
  TResourceVectorFont = class(TBGRAVectorizedFont)
    FontSource: String;
    constructor Create(const ResourceName: String);
    destructor Destroy(); override;
  end;

implementation

function GetFontName(const Stream: TStream): String;
var
  FontFace: TT_Face;
  Platform, Encoding, Language, NameID: Integer;
  I: Integer;
begin
  Result := '';
  FontFace := Default(TT_Face);
  if TT_Init_FreeType() <> TT_Err_Ok then Exit();
  if TT_Open_Face(Stream, False, FontFace) <> TT_Err_Ok then Exit();
  for I := 0 to TT_Get_Name_Count(FontFace) - 1 do begin
    if TT_Get_Name_ID(FontFace, I, Platform, Encoding, Language, NameID) <> TT_Err_Ok then continue;
    if NameID = 1 then begin // Font family entry
      Result := TT_Get_Name_String(FontFace, I);
      break;
    end;
  end;
  TT_Done_FreeType();
end;

{$ifdef Windows}

function AddFontMemResourceEx(pFileView: PVOID; cjSize: DWORD; pvResrved: PVOID; pNumFonts: PDWORD): HANDLE; stdcall;
         external 'gdi32.dll' name 'AddFontMemResourceEx';
function RemoveFontMemResourceEx(h: HANDLE): BOOL; stdcall;
         external 'gdi32.dll' name 'RemoveFontMemResourceEx';

function LoadFontResource(const ResourceName: String; out FontName: String): String;
var
  Stream: TResourceStream = Nil;
  FontCount: DWORD;
begin
  Result := '0';
  try
    Stream := TResourceStream.Create(HINSTANCE(), ResourceName, PChar(RT_RCDATA));
    FontName := GetFontName(Stream);
    Result := AddFontMemResourceEx(Stream.Memory, Stream.Size, Nil, @FontCount).ToString();
  finally
    Stream.Free();
  end;
end;

function UnloadFontResource(const FontSource: String): Boolean;
var
  HandleValue: Int64;
begin
  if not TryStrToInt64(FontSource, HandleValue) then Exit(False);
  Result := RemoveFontMemResourceEx(THandle(HandleValue));
end;

{$else}

function LoadFontResource(const ResourceName: String; out FontName: String): String;
var
  Stream: TResourceStream = Nil;
begin
  Result := Format('%s/.local/share/fonts/temp_%s', [
    ExcludeTrailingPathDelimiter(GetUserDir()),
    MD5Print(MD5String('resttf_' + ResourceName))
  ]).Replace('/', DirectorySeparator);
  try
    ForceDirectories(ExtractFileDir(Result));
    Stream := TResourceStream.Create(HINSTANCE(), ResourceName, PChar(RT_RCDATA));
    try
      FontName := GetFontName(Stream);
      Stream.SaveToFile(Result);
    except Result := '';
    end;
  finally
    Stream.Free();
  end;
end;

function UnloadFontResource(const FontSource: String): Boolean;
begin
  if not FileExists(FontSource) then Exit(False);
  Result := DeleteFile(FontSource);
end;

{$endif}

// #########################################################
// ###                TResourceVectorFont                ###
// #########################################################

constructor TResourceVectorFont.Create(const ResourceName: String);
var
  FontName: String;
begin
  inherited Create();
  FontSource := LoadFontResource(ResourceName, FontName);
  Name := FontName;
end;

destructor TResourceVectorFont.Destroy();
begin
  UnloadFontResource(FontSource);
  inherited Destroy();
end;

end.
