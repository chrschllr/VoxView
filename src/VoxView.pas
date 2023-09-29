unit VoxView;

{$mode objfpc}{$H+}
{$modeswitch advancedrecords}
{$modeswitch nestedprocvars}
{$modeswitch typehelpers}

interface

uses
  {$ifdef Windows} Windows {$else} UnixType, Linux {$endif},
  Classes, SysUtils, StrUtils, Math, DateUtils, Resource, LResources,
  Forms, Controls, Dialogs, Graphics, LCLType,
  GL, GLext, OpenGLContext,
  BGRABitmapTypes, BGRABitmap, BGRAVectorize,
  ResourceTTF;

{$define INCLUDE_SECTION_INTERFACE}
{$i VoxUtils.inc}
{$i VoxTexture.inc}
{$i VoxShader.inc}
{$i VoxMesh.inc}
{$i VoxMeshFactory.inc}
{$i VoxRenderer.inc}
{$i VoxFramebuffer.inc}
{$i VoxScene.inc}
{$i VoxViewControl.inc}
{$undef INCLUDE_SECTION_INTERFACE}

implementation

{$define INCLUDE_SECTION_IMPLEMENTATION}
{$i VoxUtils.inc}
{$i VoxTexture.inc}
{$i VoxShader.inc}
{$i VoxMesh.inc}
{$i VoxMeshFactory.inc}
{$i VoxRenderer.inc}
{$i VoxFramebuffer.inc}
{$i VoxScene.inc}
{$i VoxViewControl.inc}
{$undef INCLUDE_SECTION_IMPLEMENTATION}

var
  ParamIndex: Integer;

initialization
begin
  for ParamIndex := 1 to ParamCount do begin
    if ParamStr(ParamIndex) <> '--voxview-log-errors' then continue;
    LogStoredErrors := True;
    Assign(LogFile, GetTempDir(True) + 'voxview.log');
    Rewrite(LogFile);
    Close(LogFile);
    break;
  end;
  {$define INCLUDE_SECTION_INITIALIZATION}
  {$i VoxUtils.inc}
  {$i VoxShader.inc}
  {$i VoxMeshFactory.inc}
  {$undef INCLUDE_SECTION_INITIALIZATION}
end;

finalization
begin
  {$define INCLUDE_SECTION_FINALIZATION}
  {$i VoxMeshFactory.inc}
  {$undef INCLUDE_SECTION_FINALIZATION}
end;

end.

