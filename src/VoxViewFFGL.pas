unit VoxViewFFGL;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, GL,
  VoxView;

procedure glBegin(Mode: GLenum);
procedure glCallList({%H-}List: GLuint);
procedure glColor3f(Red, Green, Blue: GLfloat);
procedure glColor4ub(Red, Green, Blue, Alpha: GLubyte);
procedure glDeleteLists({%H-}List: GLint; {%H-}Range: GLuint);
procedure glEnd();
procedure glEndList();
function glGenLists(Range: GLsizei): GLuint;
procedure glMaterialfv({%H-}Face, {%H-}PName: GLenum; const {%H-}Params: PGLfloat);
procedure glNewList(List: GLuint; {%H-}Mode: GLenum);
procedure glNormal3d(NX, NY, NZ: GLdouble);
procedure glVertex3f(X, Y, Z: GLfloat);

procedure glMeshID(ID: Integer);
procedure glAutoNormals(AutomaticNormals: Boolean);
procedure glBreakMesh();
procedure glAddExternalMesh(const {%H-}Mesh: TVoxMesh);
function glGetMeshes(): TVoxMeshArray;
procedure glClearMeshes();
procedure glGlobalScale(Factor: Double);

type
  TVertexBuffer = specialize TArrayList<TVertex>;
  TElementBuffer = specialize TArrayList<GLuint>;
  TVoxMeshList = specialize TArrayList<TVoxMesh>;
  TVoxFixedGL = record
    List, ListIndex: SizeInt;
    Color: TColor4;
    Normal: TVec3;
    Primitive: record
      Mode, DrawType: GLenum;
      VertexCount: GLsizei;
    end;
    VertexBuffer: TVertexBuffer;
    ElementBuffer: TElementBuffer;
    Meshes: TVoxMeshList;
    GlobalScale: Double;
    AutoNormals: Boolean;
    MeshID: Integer;
  end;

var
  VoxGL: TVoxFixedGL;

implementation

// #########################################################
// ###               OpenGL State Machine                ###
// #########################################################

procedure glBegin(Mode: GLenum);
var
  DrawType: GLenum;
begin
  case Mode of
    GL_LINE, GL_LINE_STRIP: DrawType := GL_LINES;
    GL_TRIANGLES, GL_QUADS, GL_POLYGON: DrawType := GL_TRIANGLES;
    else DrawType := GL_POINTS;
  end;
  if VoxGL.Primitive.DrawType <> DrawType then glBreakMesh();
  VoxGL.Primitive.Mode := Mode;
  VoxGL.Primitive.DrawType := DrawType;
  VoxGL.Primitive.VertexCount := 0;
end;

procedure glCallList(List: GLuint);
begin
end;

procedure glColor3f(Red, Green, Blue: GLfloat);
begin
  VoxGL.Color := Color4(Red, Green, Blue, 1.0);
end;

procedure glColor4ub(Red, Green, Blue, Alpha: GLubyte);
begin
  VoxGL.Color := Color4(Red, Green, Blue, Alpha);
end;

procedure glDeleteLists(List: GLint; Range: GLuint);
begin
end;

procedure glEnd();
var
  I: Integer;
  Em0: SizeInt;
begin
  Em0 := VoxGL.VertexBuffer.Count - VoxGL.Primitive.VertexCount;
  case VoxGL.Primitive.Mode of
    GL_LINE_STRIP: for I := 0 to VoxGL.Primitive.VertexCount - 2 do begin
      VoxGL.ElementBuffer.Push([0 + Em0, 1 + Em0]);
      Em0 += 1;
    end;
    GL_TRIANGLES: for I := 0 to (VoxGL.Primitive.VertexCount div 3) - 1 do begin
      VoxGL.ElementBuffer.Push([0 + Em0, 1 + Em0, 2 + Em0]);
      Em0 += 3;
    end;
    GL_QUADS: for I := 0 to (VoxGL.Primitive.VertexCount div 4) - 1 do begin
      VoxGL.ElementBuffer.Push([0 + Em0, 1 + Em0, 2 + Em0, 2 + Em0, 3 + Em0, 0 + Em0]);
      Em0 += 4;
    end;
    GL_POLYGON: begin // Interpret as triangle fan
      for I := 0 to (VoxGL.Primitive.VertexCount - 2) - 1 do begin
        VoxGL.ElementBuffer.Push([0 + Em0, I + 1 + Em0, I + 2 + Em0]);
      end;
    end;
  end;
end;

procedure glEndList();
begin
end;

function glGenLists(Range: GLsizei): GLuint;
begin
  if Range = 0 then Exit(0);
  Result := VoxGL.ListIndex + 1;
  VoxGL.ListIndex += Range;
end;

procedure glMaterialfv(Face, PName: GLenum; const Params: PGLfloat);
begin
end;

procedure glNewList(List: GLuint; Mode: GLenum);
begin
  VoxGL.List := List;
  VoxGL.Primitive.Mode := GL_INVALID_ENUM; // Force new mesh on glBegin()
end;

procedure glNormal3d(NX, NY, NZ: GLdouble);
begin
  VoxGL.Normal := Vec3(NX, NY, NZ);
end;

procedure glVertex3f(X, Y, Z: GLfloat);
var
  Vertex: TVertex;
begin
  Vertex.Position := Vec3(X, Y, Z) * VoxGL.GlobalScale;
  Vertex.Normal := VoxGL.Normal;
  Vertex.Color := VoxGL.Color;
  VoxGL.VertexBuffer.Push(Vertex);
  VoxGL.Primitive.VertexCount += 1;
end;

procedure glMeshID(ID: Integer);
begin
  VoxGL.MeshID := ID;
end;

procedure glAutoNormals(AutomaticNormals: Boolean);
begin
  VoxGL.AutoNormals := AutomaticNormals;
end;

procedure glBreakMesh();
var
  Mesh: TVoxMesh;
begin
  if VoxGL.ElementBuffer.Count = 0 then Exit();
  Mesh := TVoxMesh.Create();
  Mesh.LoadFromBuffers(VoxGL.VertexBuffer.Items, VoxGL.ElementBuffer.Items);
  Mesh.DrawType := VoxGL.Primitive.DrawType;
  Mesh.ID := VoxGL.MeshID;
  if VoxGL.AutoNormals and (Mesh.DrawType = GL_TRIANGLES) then Mesh.RecalculateNormals();
  VoxGL.VertexBuffer.Clear();
  VoxGL.ElementBuffer.Clear();
  VoxGL.Meshes.Push(Mesh);
end;

procedure glAddExternalMesh(const Mesh: TVoxMesh);
begin
  Mesh.ID := VoxGL.MeshID;
  VoxGL.Meshes.Push(Mesh);
end;

function glGetMeshes(): TVoxMeshArray;
begin
  glBreakMesh();
  Result := VoxGL.Meshes.Items;
end;

procedure glClearMeshes();
begin
  VoxGL.Meshes.Clear();
end;

procedure glGlobalScale(Factor: Double);
begin
  VoxGL.GlobalScale := Factor;
end;

initialization
begin
  VoxGL.ListIndex := 0;
  VoxGL.Primitive.Mode := GL_INVALID_ENUM;
  VoxGL.Primitive.DrawType := GL_INVALID_ENUM;
  VoxGL.VertexBuffer := TVertexBuffer.Create();
  VoxGL.ElementBuffer := TElementBuffer.Create();
  VoxGL.Meshes := TVoxMeshList.Create();
  VoxGL.GlobalScale := 1.0;
end;

finalization
begin
  VoxGL.VertexBuffer.Free();
  VoxGL.ElementBuffer.Free();
  VoxGL.Meshes.Free();
end;

end.
