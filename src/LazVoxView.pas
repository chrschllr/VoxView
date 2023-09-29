{ This file was automatically created by Lazarus. Do not edit!
  This source is only used to compile and install the package.
 }

unit LazVoxView;

{$warn 5023 off : no warning about unused units}
interface

uses
  VoxView, VoxViewFFGL, ResourceTTF, LazarusPackageIntf;

implementation

procedure Register;
begin
  RegisterUnit('VoxView', @VoxView.Register);
end;

initialization
  RegisterPackage('LazVoxView', @Register);
end.
