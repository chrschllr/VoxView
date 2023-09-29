{interface}
{$ifdef INCLUDE_SECTION_INTERFACE}

const
  ARRAY_LIST_DEFAULT_SIZE = 256; // Size in bytes
  LINEAR_MAP_DEFAULT_SIZE = 256;

type
  TArrayListEventType = (alAdd, alRemove, alClear, alCustom);
  generic TArrayList<T> = class
  type
    PT = ^T;
    TTArray = array of T;
    TArrayListOnChange = function(const Sender: TArrayList; EventType: TArrayListEventType;
                                  Index: SizeInt; var Value: T; const CustomData: Pointer = Nil): Boolean of object;
    PObject = ^TObject;
    TArrayListEnumerator = class
      private
        ArrayList: TArrayList;
        EnumeratorIndex: Integer;
        function GetCurrent(): T;
      public
        constructor Create(Parent: TArrayList);
        function GetEnumerator(): TArrayListEnumerator;
        function MoveNext(): Boolean;
        property Current: T read GetCurrent;
    end;
  private
    FItems: TTArray;
    FIndex: SizeInt;
    function GetItems(): TTArray;
    function GetSlice(StartIndex, ElementCount: SizeInt): TTArray;
    function GetValue(ItemIndex: SizeInt): T; inline;
    procedure PutValue(ItemIndex: SizeInt; const Value: T); inline;
    function GetPointer(ItemIndex: SizeInt): PT; inline;
    function GetFirstPointer(): PT; inline;
    function GetLastPointer(): PT; inline;
    function DoAdd(const Item: T): Boolean;
    function DoRemoveIndex(Index: SizeInt): Boolean;
    function DoInsert(Index: Integer; const Item: T): Integer;
    function DoOnChange(EventType: TArrayListEventType; Index: SizeInt;
                        var Value: T; const CustomData: Pointer = Nil): Boolean;
    procedure Grow();
  public
    EventsEnabled: Boolean;
    OnChange: TArrayListOnChange;
    constructor Create(InitalSize: SizeInt = ARRAY_LIST_DEFAULT_SIZE);
    procedure FreeItems(FreeSelf: Boolean = False);
    procedure Clear(InitalSize: SizeInt = ARRAY_LIST_DEFAULT_SIZE);
    function IndexOf(const Item: T): Integer;
    function Contains(const Item: T): Boolean;
    function Push(const Item: T): Boolean;
    function Push(const Arr: TTArray): SizeInt; overload;
    function Push(const List: TArrayList): SizeInt; overload;
    function Insert(Index: Integer; const Item: T): Integer;
    function RemoveIndex(Index: SizeInt): Boolean;
    function Remove(const Item: T): Boolean;
    function RemoveAll(): SizeInt;
    function Pop(): T;
    function GetEnumerator(): TArrayListEnumerator; inline;
    property Count: SizeInt read FIndex;
    property Items: TTArray read GetItems;
    property ItemAt[ItemIndex: SizeInt]: T read GetValue write PutValue; default;
    property P[ItemIndex: SizeInt]: PT read GetPointer;
    property Slice[StartIndex, ElementCount: SizeInt]: TTArray read GetSlice;
    property First: PT read GetFirstPointer;
    property Last: PT read GetLastPointer;
    property Index: SizeInt read FIndex;
  end;
  generic TLinearMap<K, V> = class
  type
    PV = ^V;
    TKArray = array of K;
    TVArray = array of V;
    TKeyValuePair = record
      Key: K;
      Value: V;
    end;
    PObject = ^TObject;
    TLinearMapEnumerator = class
      private
        LinearMap: TLinearMap;
        EnumeratorIndex: Integer;
        function GetCurrent(): TKeyValuePair;
      public
        constructor Create(Parent: TLinearMap);
        function GetEnumerator(): TLinearMapEnumerator; inline;
        function MoveNext(): Boolean; inline;
        property Current: TKeyValuePair read GetCurrent;
    end;
  private
    FKeys: TKArray;
    FValues: TVArray;
    FIndex: SizeInt;
    function GetKeys(): TKArray;
    function GetValues(): TVArray;
    function GetPointer(const Key: K): PV;
    function GetPair(Index: Integer): TKeyValuePair;
    procedure DoPut(const Key: K; const Value: V);
    procedure Grow();
  public
    constructor Create(InitialSize: SizeInt = LINEAR_MAP_DEFAULT_SIZE);
    procedure FreeItems(FreeSelf: Boolean = False; FreeValues: Boolean = True; FreeKeys: Boolean = False);
    procedure Clear(InitialSize: SizeInt = LINEAR_MAP_DEFAULT_SIZE);
    function Get(const Key: K): V;
    function Put(const Key: K; const Value: V): Integer;
    function IndexOfKey(const Key: K): Integer;
    function IndexOfValue(const Value: V): Integer;
    function FindKey(const Key: K; out Index: Integer): Boolean;
    function FindValue(const Value: V; out Index: Integer): Boolean;
    function HasKey(const Key: K): Boolean;
    function HasValue(const Value: V): Boolean;
    function RemoveAt(Index: Integer): Boolean;
    function Remove(const Key: K): Boolean;
    function GetEnumerator(): TLinearMapEnumerator; inline;
    property Count: SizeInt read FIndex;
    property Keys: TKArray read GetKeys;
    property Values: TVArray read GetValues;
    property ValueOf[const Key: K]: V read Get write DoPut; default;
    property P[const Key: K]: PV read GetPointer;
    property Pairs[const Index: Integer]: TKeyValuePair read GetPair;
  end;
  TVec2 = packed record
    X, Y: GLfloat;
    function IsNaN(): Boolean;
    function IsInfinite(): Boolean;
    function LengthSqr(): GLfloat;
    function Length(): GLfloat;
    function DistTo(const VecB: TVec2): GLfloat;
    function Normal(): TVec2;
    function Dot(const VecB: TVec2): GLfloat;
    function RightAngled(): TVec2;
    function ToString(): String;
  end;
  PVec2 = ^TVec2;
  TVec3 = packed record
    X, Y, Z: GLfloat;
    function IsNaN(): Boolean;
    function IsInfinite(): Boolean;
    function LengthSqr(): GLfloat;
    function Length(): GLfloat;
    function DistTo(const VecB: TVec3): GLfloat;
    function Normal(): TVec3;
    function Dot(const VecB: TVec3): GLfloat;
    function Cross(const VecB: TVec3): TVec3;
    function RightAngled(): TVec3;
    function ToString(): String;
  end;
  PVec3 = ^TVec3;
  TVec4 = packed record
    X, Y, Z, W: GLfloat;
    function IsNaN(): Boolean;
    function IsInfinite(): Boolean;
    function LengthSqr(): GLfloat;
    function Length(): GLfloat;
    function DistTo(const VecB: TVec4): GLfloat;
    function ToString(): String;
  end;
  PVec4 = ^TVec4;
  TColor4 = packed record
    R, G, B, A: GLfloat;
    function GammaCorrect(Gamma: GLfloat): TColor4;
    function Luma(): GLfloat;
    function ToString(): String;
  end;
  PColor4 = ^TColor4;
  // [column / x][row / y] (column-major order) (as in GLSL)
  // Example: printed as specified above, individually cast to byte stream below
  // [ 1.00000 | 0.00000 |  0.00000 | 0.00000 ]
  // [ 2.00000 | 1.00000 |  0.00000 | 0.00000 ]
  // [ 3.00000 | 0.00000 | -1.00000 | 0.00000 ]
  // [ 4.00000 | 0.00000 |  0.00000 | 1.00000 ]
  // 01 02 03 04 00 01 00 00 00 00 FF 00 00 00 00 01
  TMatrix4 = packed array[0..3, 0..3] of GLfloat;
  PMatrix4 = ^TMatrix4;
  TMatrix4Helper = type helper for TMatrix4
    function Inverse(): TMatrix4;
    function Inverse3x3(): TMatrix4;
    function Transpose(): TMatrix4;
    function ToString(): String;
  end;

const
  VEC2_X: TVec2               = (X: 1.0; Y: 0.0);
  VEC2_Y: TVec2               = (X: 0.0; Y: 1.0);
  VEC3_X: TVec3               = (X: 1.0; Y: 0.0; Z: 0.0);
  VEC3_Y: TVec3               = (X: 0.0; Y: 1.0; Z: 0.0);
  VEC3_Z: TVec3               = (X: 0.0; Y: 0.0; Z: 1.0);
  COLOR4_NONE: TColor4        = (R: 0.0; G: 0.0; B: 0.0; A: 0.0);
  COLOR4_NONE_BLACK: TColor4  = (R: 0.0; G: 0.0; B: 0.0; A: 0.0);
  COLOR4_NONE_WHITE: TColor4  = (R: 1.0; G: 1.0; B: 1.0; A: 0.0);
  COLOR4_ULTRAWHITE: TColor4  = (R: 2.0; G: 2.0; B: 2.0; A: 1.0);
  COLOR4_BLACK: TColor4       = (R: 0.0; G: 0.0; B: 0.0; A: 1.0);
  COLOR4_WHITE: TColor4       = (R: 1.0; G: 1.0; B: 1.0; A: 1.0);
  COLOR4_GRAY: TColor4        = (R: 0.5; G: 0.5; B: 0.5; A: 1.0);
  COLOR4_RED: TColor4         = (R: 1.0; G: 0.0; B: 0.0; A: 1.0);
  COLOR4_GREEN: TColor4       = (R: 0.0; G: 1.0; B: 0.0; A: 1.0);
  COLOR4_BLUE: TColor4        = (R: 0.0; G: 0.0; B: 1.0; A: 1.0);
  COLOR4_YELLOW: TColor4      = (R: 1.0; G: 1.0; B: 0.0; A: 1.0);
  COLOR4_CYAN: TColor4        = (R: 0.0; G: 1.0; B: 1.0; A: 1.0);
  COLOR4_MAGENTA: TColor4     = (R: 1.0; G: 0.0; B: 1.0; A: 1.0);
  MATRIX4_IDENTITY: TMatrix4  = (
    (1.0, 0.0, 0.0, 0.0),
    (0.0, 1.0, 0.0, 0.0),
    (0.0, 0.0, 1.0, 0.0),
    (0.0, 0.0, 0.0, 1.0)
  );

function Vec2(AX, AY: GLfloat): TVec2;
function Vec2(const VecA: TVec3): TVec2; overload;
function Vec2(const VecA: TVec4): TVec2; overload;
function Vec2(const Axis: String): TVec2; overload;
operator :=(const PointA: TPoint): TVec3;
operator =(const VecA, VecB: TVec2): Boolean;
operator +(const VecA, VecB: TVec2): TVec2;
operator -(const VecA, VecB: TVec2): TVec2;
operator -(const VecA: TVec2): TVec2;
operator *(const VecA, VecB: TVec2): TVec2;
operator *(const VecA: TVec2; FacB: GLfloat): TVec2;
operator /(const VecA, VecB: TVec2): TVec2;
operator /(const VecA: TVec2; DivB: GLfloat): TVec2;

function Vec3(AX, AY, AZ: GLfloat): TVec3;
function Vec3(const VecA: TVec4): TVec3; overload;
function Vec3(const VecA: TVec2; AZ: GLfloat): TVec3; overload;
function Vec3(const Axis: String): TVec3; overload;
operator =(const VecA, VecB: TVec3): Boolean;
operator +(const VecA, VecB: TVec3): TVec3;
operator -(const VecA, VecB: TVec3): TVec3;
operator -(const VecA: TVec3): TVec3;
operator *(const VecA, VecB: TVec3): TVec3;
operator *(const VecA: TVec3; FacB: GLfloat): TVec3;
operator /(const VecA, VecB: TVec3): TVec3;
operator /(const VecA: TVec3; DivB: GLfloat): TVec3;

function Vec4(AX, AY, AZ, AW: GLfloat): TVec4;
function Vec4(const VecA: TVec2; AZ, AW: GLfloat): TVec4; overload;
function Vec4(const VecA: TVec3; AW: GLfloat): TVec4; overload;
operator :=(const ColorA: TColor4): TVec4;
operator =(const VecA, VecB: TVec4): Boolean;

function Color4(AR, AG, AB: GLfloat; AA: GLfloat = 1.0): TColor4;
function Color4(AR, AG, AB: Byte; AA: Byte = 255): TColor4; overload;
function Color4(const ColorA: TColor4; AA: GLfloat): TColor4; overload;
function Color4(const ColorA: TColor4; AA: Byte): TColor4; overload;
operator :=(const VecA: TVec4): TColor4;
operator :=(const ColorA: TColor): TColor4;
operator :=(const ColorA: TColor4): TColor;
operator =(const ColorA, ColorB: TColor4): Boolean;
operator +(const ColorA, ColorB: TColor4): TColor4;
operator -(const ColorA, ColorB: TColor4): TColor4;
operator *(const ColorA, ColorB: TColor4): TColor4;
operator *(const ColorA: TColor4; FacB: GLfloat): TColor4;

function Matrix4(const Rows: array of GLfloat): TMatrix4; overload;
function RotationMatrix(const Axis: TVec3; AngleRad: GLfloat): TMatrix4;
function ScaleMatrix(const Scale: TVec3): TMatrix4;
function TranslationMatrix(const Translation: TVec3): TMatrix4;
function ReorientationMatrix(const VecAxisX, VecAxisY, VecAxisZ: TVec3): TMatrix4;
function FlatteningMatrix(const VecAxis: TVec3): TMatrix4;
function ReflectionMatrix(const VecAxis: TVec3): TMatrix4;
function TransformationMatrix(const Operations: array of TMatrix4): TMatrix4;
operator =(const MatA, MatB: TMatrix4): Boolean;
operator +(const MatA, MatB: TMatrix4): TMatrix4;
operator -(const MatA, MatB: TMatrix4): TMatrix4;
operator *(const MatA, MatB: TMatrix4): TMatrix4;
operator *(const MatA: TMatrix4; FacB: GLfloat): TMatrix4;
operator *(const MatA: TMatrix4; const VecB: TVec3): TVec3;
operator *(const MatA: TMatrix4; const VecB: TVec4): TVec4;

procedure PrintElementBuffer(Elements: array of GLuint; Stride: Integer = 3; LineEndingStride: Integer = -1);
procedure WriteF(const Str: String; const Values: array of const; AddNewLine: Boolean = True);

var
  VOX_FORMAT: TFormatSettings;

{$endif}

{implementation}
{$ifdef INCLUDE_SECTION_IMPLEMENTATION}

// #########################################################
// ###                    TArrayList                     ###
// #########################################################

function TArrayList.TArrayListEnumerator.GetEnumerator(): TArrayListEnumerator;
begin
  Result := Self;
end;

constructor TArrayList.TArrayListEnumerator.Create(Parent: TArrayList);
begin
  ArrayList := Parent;
  EnumeratorIndex := -1; // MoveNext is called immediately
end;

function TArrayList.TArrayListEnumerator.MoveNext(): Boolean;
begin
  EnumeratorIndex += 1;
  Result := EnumeratorIndex < ArrayList.Count;
end;

function TArrayList.TArrayListEnumerator.GetCurrent(): T;
begin
  Result := ArrayList.FItems[EnumeratorIndex];
end;

function TArrayList.GetItems(): TTArray;
begin
  SetLength(FItems, Count);
  Result := FItems;
end;

function TArrayList.GetSlice(StartIndex, ElementCount: SizeInt): TTArray;
var
  I: SizeInt;
begin
  Result := Default(TTArray);
  SetLength(Result, ElementCount);
  for I := 0 to ElementCount - 1 do Result[I] := FItems[StartIndex + I];
end;

function TArrayList.GetValue(ItemIndex: SizeInt): T;
begin
  Result := FItems[ItemIndex];
end;

procedure TArrayList.PutValue(ItemIndex: SizeInt; const Value: T);
begin
  FItems[ItemIndex] := Value;
end;

function TArrayList.GetPointer(ItemIndex: SizeInt): PT;
begin
  Result := @FItems[ItemIndex];
end;

function TArrayList.GetFirstPointer(): PT;
begin
  Result := @FItems[0];
end;

function TArrayList.GetLastPointer(): PT;
begin
  Result := @FItems[FIndex - 1];
end;

function TArrayList.DoAdd(const Item: T): Boolean;
begin
  Result := True;
  if Count >= Length(FItems) then Grow();
  FItems[FIndex] := Item;
  FIndex += 1;
  if not DoOnChange(alAdd, FIndex - 1, FItems[FIndex - 1]) then begin
    FIndex -= 1;
    Result := False;
  end;
end;

function TArrayList.DoRemoveIndex(Index: SizeInt): Boolean;
var
  I: SizeInt;
begin
  if (Index >= 0) and (Index < Count) then begin
    FIndex -= 1;
    if not DoOnChange(alRemove, Index, FItems[Index]) then begin
      FIndex += 1;
      Exit(False);
    end;
    for I := Index to Count - 1 do FItems[I] := FItems[I + 1];
    Result := True;
  end else Result := False;
end;

function TArrayList.DoInsert(Index: Integer; const Item: T): Integer;
var
  I: SizeInt;
begin
  Index := Max(0, Min(Index, Count));
  if Count >= Length(FItems) then Grow();
  FIndex += 1;
  for I := Count - 1 downto Index + 1 do FItems[I] := FItems[I - 1];
  FItems[Index] := Item;
  if not DoOnChange(alAdd, Index, FItems[Index]) then begin
    for I := Index to Count - 2 do FItems[I] := FItems[I + 1];
    FIndex -= 1;
    Exit(-1);
  end;
  Result := Index;
end;

function TArrayList.DoOnChange(EventType: TArrayListEventType; Index: SizeInt;
                               var Value: T; const CustomData: Pointer = Nil): Boolean;
begin
  if (OnChange <> Nil) and EventsEnabled then begin
    Result := OnChange(Self, EventType, Index, Value, CustomData);
  end else Result := True;
end;

procedure TArrayList.Grow();
begin
  SetLength(FItems, Trunc(Length(FItems) * 1.5) + 1);
end;

constructor TArrayList.Create(InitalSize: SizeInt = ARRAY_LIST_DEFAULT_SIZE);
begin
  Clear(InitalSize);
  EventsEnabled := True;
end;

procedure TArrayList.FreeItems(FreeSelf: Boolean = False);
var
  I: Integer;
begin
  for I := 0 to Count - 1 do (PObject(@FItems[I]))^.Free();
  if not FreeSelf then begin
    FIndex := 0;
    Clear();
  end else Self.Free();
end;

procedure TArrayList.Clear(InitalSize: SizeInt = ARRAY_LIST_DEFAULT_SIZE);
begin
  if not DoOnChange(alClear, 0, FItems[0], Nil) then Exit();
  FIndex := 0;
  SetLength(FItems, InitalSize div SizeOf(FItems[0]) + 1);
end;

function TArrayList.IndexOf(const Item: T): Integer;
var
  I: SizeInt;
begin
  Result := -1;
  for I := 0 to Count - 1 do begin
    if FItems[I] = Item then Exit(I);
  end;
end;

function TArrayList.Contains(const Item: T): Boolean;
begin
  Result := IndexOf(Item) >= 0;
end;

function TArrayList.Push(const Item: T): Boolean;
begin
  Result := DoAdd(Item);
end;

function TArrayList.Push(const Arr: TTArray): SizeInt;
var
  I: SizeInt;
begin
  Result := 0;
  for I := 0 to Length(Arr) - 1 do begin
    if DoAdd(Arr[I]) then Result += 1;
  end;
end;

function TArrayList.Push(const List: TArrayList): SizeInt;
var
  I: SizeInt;
begin
  Result := 0;
  for I := 0 to List.Count - 1 do begin
    if DoAdd(List[I]) then Result += 1;
  end;
end;

function TArrayList.Insert(Index: Integer; const Item: T): Integer;
begin
  Result := DoInsert(Index, Item);
end;

function TArrayList.RemoveIndex(Index: SizeInt): Boolean;
begin
  Result := DoRemoveIndex(Index);
end;

function TArrayList.Remove(const Item: T): Boolean;
begin
  Result := DoRemoveIndex(IndexOf(Item));
end;

function TArrayList.RemoveAll(): SizeInt;
var
  I: SizeInt;
begin
  Result := 0;
  for I := 0 to Count - 1 do begin
    if DoRemoveIndex(I - Result) then Result += 1;
  end;
end;

function TArrayList.Pop(): T;
var
  Popped: PT;
begin
  Result := Default(T);
  Popped := Last;
  if RemoveIndex(FIndex - 1) then Result := Popped^;
end;

function TArrayList.GetEnumerator(): TArrayListEnumerator;
begin
  Result := TArrayListEnumerator.Create(Self);
end;

// #########################################################
// ###                    TLinearMap                     ###
// #########################################################

function TLinearMap.TLinearMapEnumerator.GetEnumerator(): TLinearMapEnumerator;
begin
  Result := Self;
end;

constructor TLinearMap.TLinearMapEnumerator.Create(Parent: TLinearMap);
begin
  LinearMap := Parent;
  EnumeratorIndex := -1; // MoveNext is called immediately
end;

function TLinearMap.TLinearMapEnumerator.MoveNext(): Boolean;
begin
  EnumeratorIndex += 1;
  Result := EnumeratorIndex < LinearMap.Count;
end;

function TLinearMap.TLinearMapEnumerator.GetCurrent(): TKeyValuePair;
begin
  Result := LinearMap.Pairs[EnumeratorIndex];
end;

function TLinearMap.GetKeys(): TKArray;
begin
  SetLength(FKeys, FIndex);
  Result := FKeys;
end;

function TLinearMap.GetValues(): TVArray;
begin
  SetLength(FValues, FIndex);
  Result := FValues;
end;

{$ifdef TLINEARMAP_CREATE_ACCESSED}

function TLinearMap.GetPointer(const Key: K): PV;
var
  I: Integer;
begin
  if not FindKey(Key, I) then begin
    if FIndex >= Min(Length(FKeys), Length(FValues)) then Grow();
    FKeys[FIndex] := Key;
    FValues[FIndex] := Default(V);
    I := FIndex;
    FIndex += 1;
  end;
  Result := @FValues[I];
end;

{$else}

function TLinearMap.GetPointer(const Key: K): PV;
var
  I: Integer;
begin
  if FindKey(Key, I) then begin
    Result := @FValues[I];
  end else Result := Nil;
end;

{$endif}

function TLinearMap.GetPair(Index: Integer): TLinearMap.TKeyValuePair;
begin
  Result := Default(TLinearMap.TKeyValuePair);
  if (Index < 0) or (Index >= Count) then Exit();
  Result.Key := FKeys[Index];
  Result.Value := FValues[Index];
end;

procedure TLinearMap.DoPut(const Key: K; const Value: V);
begin
  Put(Key, Value);
end;

procedure TLinearMap.Grow();
var
  NewLength: SizeInt;
begin
  NewLength := Trunc(Min(Length(FKeys), Length(FValues)) * 1.5) + 1;
  SetLength(FKeys, NewLength);
  SetLength(FValues, NewLength);
end;

constructor TLinearMap.Create(InitialSize: SizeInt = LINEAR_MAP_DEFAULT_SIZE);
begin
  Clear(InitialSize);
end;

procedure TLinearMap.FreeItems(FreeSelf: Boolean = False; FreeValues: Boolean = True; FreeKeys: Boolean = False);
var
  I: Integer;
begin
  if FreeKeys then for I := 0 to Count - 1 do (PObject(@FKeys[I]))^.Free();
  if FreeValues then for I := 0 to Count - 1 do (PObject(@FValues[I]))^.Free();
  if FreeSelf then Free() else Clear();
end;

procedure TLinearMap.Clear(InitialSize: SizeInt = LINEAR_MAP_DEFAULT_SIZE);
begin
  FIndex := 0;
  SetLength(FKeys, InitialSize div SizeOf(FKeys[0]) + 1);
  SetLength(FValues, InitialSize div SizeOf(FValues[0]) + 1);
end;

function TLinearMap.Get(const Key: K): V;
begin
  Result := GetPointer(Key)^;
end;

function TLinearMap.Put(const Key: K; const Value: V): Integer;
begin
  if not FindKey(Key, Result) then begin
    if FIndex >= Min(Length(FKeys), Length(FValues)) then Grow();
    FKeys[FIndex] := Key;
    FValues[FIndex] := Value;
    Result := FIndex;
    FIndex += 1;
  end else FValues[Result] := Value;
end;

function TLinearMap.IndexOfKey(const Key: K): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to FIndex - 1 do begin
    if FKeys[I] = Key then Exit(I);
  end;
end;

function TLinearMap.IndexOfValue(const Value: V): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to FIndex - 1 do begin
    if FValues[I] = Value then Exit(I);
  end;
end;

function TLinearMap.FindKey(const Key: K; out Index: Integer): Boolean;
var
  I: Integer;
begin
  I := IndexOfKey(Key);
  Result := I >= 0;
  if Result then Index := I;
end;

function TLinearMap.FindValue(const Value: V; out Index: Integer): Boolean;
var
  I: Integer;
begin
  I := IndexOfValue(Value);
  Result := I >= 0;
  if Result then Index := I;
end;

function TLinearMap.HasKey(const Key: K): Boolean;
begin
  Result := IndexOfKey(Key) >= 0;
end;

function TLinearMap.HasValue(const Value: V): Boolean;
begin
  Result := IndexOfValue(Value) >= 0;
end;

function TLinearMap.RemoveAt(Index: Integer): Boolean;
var
  I: SizeInt;
begin
  if (Index < 0) or (Index >= Count) then Exit(False);
  for I := Index to Count - 2 do begin
    FKeys[I] := FKeys[I + 1];
    FValues[I] := FValues[I + 1];
  end;
  FIndex -= 1;
  Result := True;
end;

function TLinearMap.Remove(const Key: K): Boolean;
begin
  Result := RemoveAt(IndexOfKey(Key));
end;

function TLinearMap.GetEnumerator(): TLinearMapEnumerator;
begin
  Result := TLinearMapEnumerator.Create(Self);
end;

// #########################################################
// ###                       TVec2                       ###
// #########################################################

function Vec2(AX, AY: GLfloat): TVec2;
begin
  Result.X := AX;
  Result.Y := AY;
end;

function Vec2(const VecA: TVec3): TVec2;
begin
  Result.X := VecA.X;
  Result.Y := VecA.Y;
end;

function Vec2(const VecA: TVec4): TVec2;
begin
  Result.X := VecA.X;
  Result.Y := VecA.Y;
end;

function Vec2(const Axis: String): TVec2;
begin
  case AnsiLowerCase(Axis) of
    'x': Result := VEC2_X;
    'y': Result := VEC2_Y;
    else Result := Vec2(0.0, 0.0);
  end;
end;

operator :=(const PointA: TPoint): TVec3;
begin
  Result.X := PointA.X;
  Result.Y := PointA.Y;
end;

operator =(const VecA, VecB: TVec2): Boolean;
begin
  Result := (VecA.X = VecB.X) and (VecA.Y = VecB.Y);
end;

operator +(const VecA, VecB: TVec2): TVec2;
begin
  Result.X := VecA.X + VecB.X;
  Result.Y := VecA.Y + VecB.Y;
end;

operator -(const VecA, VecB: TVec2): TVec2;
begin
  Result.X := VecA.X - VecB.X;
  Result.Y := VecA.Y - VecB.Y;
end;

operator -(const VecA: TVec2): TVec2;
begin
  Result.X := -VecA.X;
  Result.Y := -VecA.Y;
end;

operator *(const VecA, VecB: TVec2): TVec2;
begin
  Result.X := VecA.X * VecB.X;
  Result.Y := VecA.Y * VecB.Y;
end;

operator *(const VecA: TVec2; FacB: GLfloat): TVec2;
begin
  Result.X := VecA.X * FacB;
  Result.Y := VecA.Y * FacB;
end;

operator /(const VecA, VecB: TVec2): TVec2;
begin
  Result.X := VecA.X / VecB.X;
  Result.Y := VecA.Y / VecB.Y;
end;

operator /(const VecA: TVec2; DivB: GLfloat): TVec2;
begin
  Result.X := VecA.X / DivB;
  Result.Y := VecA.Y / DivB;
end;

function TVec2.IsNaN(): Boolean;
begin
  Result := Math.IsNaN(X) or Math.IsNaN(Y);
end;

function TVec2.IsInfinite(): Boolean;
begin
  Result := Math.IsInfinite(X) or Math.IsInfinite(Y);
end;

function TVec2.LengthSqr(): GLfloat;
begin
  Result := X * X + Y * Y;
end;

function TVec2.Length(): GLfloat;
begin
  Result := Sqrt(X * X + Y * Y);
end;

function TVec2.DistTo(const VecB: TVec2): GLfloat;
begin
  Result := (VecB - Self).Length();
end;

function TVec2.Normal(): TVec2;
begin
  Result := Self / Length();
end;

function TVec2.Dot(const VecB: TVec2): GLfloat;
begin
  Result := X * VecB.X + Y * VecB.Y;
end;

function TVec2.RightAngled(): TVec2;
begin
  Result := Vec2(-Y, X);
end;

function TVec2.ToString(): String;
begin
  Result := Format('TVec2 { x: %.4f, y: %.4f }', [X, Y], VOX_FORMAT);
end;

// #########################################################
// ###                       TVec3                       ###
// #########################################################

function Vec3(AX, AY, AZ: GLfloat): TVec3;
begin
  Result.X := AX;
  Result.Y := AY;
  Result.Z := AZ;
end;

function Vec3(const VecA: TVec4): TVec3;
begin
  Result.X := VecA.X;
  Result.Y := VecA.Y;
  Result.Z := VecA.Z;
end;

function Vec3(const VecA: TVec2; AZ: GLfloat): TVec3;
begin
  Result.X := VecA.X;
  Result.Y := VecA.Y;
  Result.Z := AZ;
end;

function Vec3(const Axis: String): TVec3;
begin
  case AnsiLowerCase(Axis) of
    'x': Result := VEC3_X;
    'y': Result := VEC3_Y;
    'z': Result := VEC3_Z;
    else Result := Vec3(0.0, 0.0, 0.0);
  end;
end;

operator =(const VecA, VecB: TVec3): Boolean;
begin
  Result := (VecA.X = VecB.X) and (VecA.Y = VecB.Y) and (VecA.Z = VecB.Z);
end;

operator +(const VecA, VecB: TVec3): TVec3;
begin
  Result.X := VecA.X + VecB.X;
  Result.Y := VecA.Y + VecB.Y;
  Result.Z := VecA.Z + VecB.Z;
end;

operator -(const VecA, VecB: TVec3): TVec3;
begin
  Result.X := VecA.X - VecB.X;
  Result.Y := VecA.Y - VecB.Y;
  Result.Z := VecA.Z - VecB.Z;
end;

operator -(const VecA: TVec3): TVec3;
begin
  Result.X := -VecA.X;
  Result.Y := -VecA.Y;
  Result.Z := -VecA.Z;
end;

operator *(const VecA, VecB: TVec3): TVec3;
begin
  Result.X := VecA.X * VecB.X;
  Result.Y := VecA.Y * VecB.Y;
  Result.Z := VecA.Z * VecB.Z;
end;

operator *(const VecA: TVec3; FacB: GLfloat): TVec3;
begin
  Result.X := VecA.X * FacB;
  Result.Y := VecA.Y * FacB;
  Result.Z := VecA.Z * FacB;
end;

operator /(const VecA, VecB: TVec3): TVec3;
begin
  Result.X := VecA.X / VecB.X;
  Result.Y := VecA.Y / VecB.Y;
  Result.Z := VecA.Z / VecB.Z;
end;

operator /(const VecA: TVec3; DivB: GLfloat): TVec3;
begin
  Result.X := VecA.X / DivB;
  Result.Y := VecA.Y / DivB;
  Result.Z := VecA.Z / DivB;
end;

function TVec3.IsNaN(): Boolean;
begin
  Result := Math.IsNaN(X) or Math.IsNaN(Y) or Math.IsNaN(Z);
end;

function TVec3.IsInfinite(): Boolean;
begin
  Result := Math.IsInfinite(X) or Math.IsInfinite(Y) or Math.IsInfinite(Z);
end;

function TVec3.LengthSqr(): GLfloat;
begin
  Result := X * X + Y * Y + Z * Z;
end;

function TVec3.Length(): GLfloat;
begin
  Result := Sqrt(X * X + Y * Y + Z * Z);
end;

function TVec3.DistTo(const VecB: TVec3): GLfloat;
begin
  Result := (VecB - Self).Length();
end;

function TVec3.Normal(): TVec3;
begin
  Result := Self / Length();
end;

function TVec3.Dot(const VecB: TVec3): GLfloat;
begin
  Result := X * VecB.X + Y * VecB.Y + Z * VecB.Z;
end;

function TVec3.Cross(const VecB: TVec3): TVec3;
begin
  Result.X := Y * VecB.Z - Z * VecB.Y;
  Result.Y := Z * VecB.X - X * VecB.Z;
  Result.Z := X * VecB.Y - Y * VecB.X;
end;

function TVec3.RightAngled(): TVec3;
begin
  if X = Y then begin
    Result := Vec3(-Z, 0.0, X)
  end else Result := Vec3(-Y, X, 0.0);
end;

function TVec3.ToString(): String;
begin
  Result := Format('TVec3 { x: %.4f, y: %.4f, z: %.4f }', [X, Y, Z], VOX_FORMAT);
end;

// #########################################################
// ###                       TVec4                       ###
// #########################################################

function Vec4(AX, AY, AZ, AW: GLfloat): TVec4;
begin
  Result.X := AX;
  Result.Y := AY;
  Result.Z := AZ;
  Result.W := AW;
end;

function Vec4(const VecA: TVec2; AZ, AW: GLfloat): TVec4;
begin
  Result.X := VecA.X;
  Result.Y := VecA.Y;
  Result.Z := AZ;
  Result.W := AW;
end;

function Vec4(const VecA: TVec3; AW: GLfloat): TVec4;
begin
  Result.X := VecA.X;
  Result.Y := VecA.Y;
  Result.Z := VecA.Z;
  Result.W := AW;
end;

operator :=(const ColorA: TColor4): TVec4;
begin
  Move(ColorA, Result{%H-}, SizeOf(Result));
end;

operator =(const VecA, VecB: TVec4): Boolean;
begin
  Result := (VecA.X = VecB.X) and (VecA.Y = VecB.Y) and (VecA.Z = VecB.Z) and (VecA.W = VecB.W);
end;

function TVec4.IsNaN(): Boolean;
begin
  Result := Math.IsNaN(X) or Math.IsNaN(Y) or Math.IsNaN(Z) or Math.IsNaN(W);
end;

function TVec4.IsInfinite(): Boolean;
begin
  Result := Math.IsInfinite(X) or Math.IsInfinite(Y) or Math.IsInfinite(Z) or Math.IsInfinite(W);
end;

function TVec4.LengthSqr(): GLfloat;
begin
  Result := X * X + Y * Y + Z * Z + W * W;
end;

function TVec4.Length(): GLfloat;
begin
  Result := Sqrt(X * X + Y * Y + Z * Z + W * W);
end;

function TVec4.DistTo(const VecB: TVec4): GLfloat;
begin
  Result := TVec4(VecB - Self).Length(); // Why is this redundant cast required?
end;

function TVec4.ToString(): String;
begin
  Result := Format('TVec4 { x: %.4f, y: %.4f, z: %.4f, w: %.4f }', [X, Y, Z, W], VOX_FORMAT);
end;

// #########################################################
// ###                      TColor4                      ###
// #########################################################

function Color4(AR, AG, AB: GLfloat; AA: GLfloat = 1.0): TColor4;
begin
  Result.R := AR;
  Result.G := AG;
  Result.B := AB;
  Result.A := AA;
end;

function Color4(AR, AG, AB: Byte; AA: Byte = 255): TColor4;
begin
  Result.R := AR / 255.0;
  Result.G := AG / 255.0;
  Result.B := AB / 255.0;
  Result.A := AA / 255.0;
end;

function Color4(const ColorA: TColor4; AA: GLfloat): TColor4;
begin
  Result := ColorA;
  Result.A := AA;
end;

function Color4(const ColorA: TColor4; AA: Byte): TColor4;
begin
  Result := ColorA;
  Result.A := AA / 255.0;
end;

operator :=(const VecA: TVec4): TColor4;
begin
  Move(VecA, Result{%H-}, SizeOf(Result));
end;

operator :=(const ColorA: TColor): TColor4;
var
  RGB: LongInt;
begin
  RGB := ColorToRGB(ColorA);
  Result := Color4(Red(RGB), Green(RGB), Blue(RGB));
end;

operator :=(const ColorA: TColor4): TColor;
begin
  Result := RGBToColor(Trunc(ColorA.R * 255), Trunc(ColorA.G * 255), Trunc(ColorA.B * 255));
end;

operator =(const ColorA, ColorB: TColor4): Boolean;
begin
  Result := (ColorA.R = ColorB.R) and (ColorA.G = ColorB.G) and (ColorA.B = ColorB.B) and (ColorA.A = ColorB.A);
end;

operator +(const ColorA, ColorB: TColor4): TColor4;
begin
  Result.R := ColorA.R + ColorB.R;
  Result.G := ColorA.G + ColorB.G;
  Result.B := ColorA.B + ColorB.B;
  Result.A := ColorA.A + ColorB.A;
end;

operator -(const ColorA, ColorB: TColor4): TColor4;
begin
  Result.R := ColorA.R - ColorB.R;
  Result.G := ColorA.G - ColorB.G;
  Result.B := ColorA.B - ColorB.B;
  Result.A := ColorA.A - ColorB.A;
end;

operator *(const ColorA, ColorB: TColor4): TColor4;
begin
  Result.R := ColorA.R * ColorB.R;
  Result.G := ColorA.G * ColorB.G;
  Result.B := ColorA.B * ColorB.B;
  Result.A := ColorA.A * ColorB.A;
end;

operator *(const ColorA: TColor4; FacB: GLfloat): TColor4;
begin
  Result.R := ColorA.R * FacB;
  Result.G := ColorA.G * FacB;
  Result.B := ColorA.B * FacB;
  Result.A := ColorA.A * FacB;
end;

function TColor4.GammaCorrect(Gamma: GLfloat): TColor4;
begin
  Result.R := Power(R, Gamma);
  Result.G := Power(G, Gamma);
  Result.B := Power(B, Gamma);
  Result.A := A;
end;

function TColor4.Luma(): GLfloat;
begin
  Result := R * 0.299 + G * 0.587 + B * 0.114;
end;

function TColor4.ToString(): String;
begin
  Result := Format('TColor4 { r: %.3f, g: %.3f, b: %.3f, a: %.3f }', [R, G, B, A], VOX_FORMAT);
end;

// #########################################################
// ###                     TMatrix4                      ###
// #########################################################

function Matrix4(const Rows: array of GLfloat): TMatrix4;
begin
  Result[0, 0] := Rows[$0]; Result[1, 0] := Rows[$1]; Result[2, 0] := Rows[$2]; Result[3, 0] := Rows[$3];
  Result[0, 1] := Rows[$4]; Result[1, 1] := Rows[$5]; Result[2, 1] := Rows[$6]; Result[3, 1] := Rows[$7];
  Result[0, 2] := Rows[$8]; Result[1, 2] := Rows[$9]; Result[2, 2] := Rows[$A]; Result[3, 2] := Rows[$B];
  Result[0, 3] := Rows[$C]; Result[1, 3] := Rows[$D]; Result[2, 3] := Rows[$E]; Result[3, 3] := Rows[$F];
end;

function RotationMatrix(const Axis: TVec3; AngleRad: GLfloat): TMatrix4;
var
  S, C, CI, XY_CI, YZ_CI, XZ_CI, X_S, Y_S, Z_S, XX, YY, ZZ: GLfloat;
begin
  S := Sin(AngleRad);
  C := Cos(AngleRad);
  CI := 1.0 - C;
  XY_CI := Axis.X * Axis.Y * CI;
  YZ_CI := Axis.Y * Axis.Z * CI;
  XZ_CI := Axis.X * Axis.Z * CI;
  X_S := Axis.X * S;
  Y_S := Axis.Y * S;
  Z_S := Axis.Z * S;
  XX := Axis.X * Axis.X;
  YY := Axis.Y * Axis.Y;
  ZZ := Axis.Z * Axis.Z;
  Result := Matrix4([
    C + XX * CI,  XY_CI - Z_S,  XZ_CI + Y_S,  0.0,
    XY_CI + Z_S,  C + YY * CI,  YZ_CI - X_S,  0.0,
    XZ_CI - Y_S,  YZ_CI + X_S,  C + ZZ * CI,  0.0,
    0.0,          0.0,          0.0,          1.0
  ]);
end;

function ScaleMatrix(const Scale: TVec3): TMatrix4;
begin
  Result := MATRIX4_IDENTITY;
  Result[0, 0] := Scale.X;
  Result[1, 1] := Scale.Y;
  Result[2, 2] := Scale.Z;
end;

function TranslationMatrix(const Translation: TVec3): TMatrix4;
begin
  Result := MATRIX4_IDENTITY;
  Result[3, 0] := Translation.X;
  Result[3, 1] := Translation.Y;
  Result[3, 2] := Translation.Z;
end;

function ReorientationMatrix(const VecAxisX, VecAxisY, VecAxisZ: TVec3): TMatrix4;
begin
  Result := Matrix4([
    VecAxisX.X, VecAxisY.X, VecAxisZ.X, 0.0,
    VecAxisX.Y, VecAxisY.Y, VecAxisZ.Y, 0.0,
    VecAxisX.Z, VecAxisY.Z, VecAxisZ.Z, 0.0,
    0.0,        0.0,        0.0,        1.0
  ]).Inverse3x3();
end;

function FlatteningMatrix(const VecAxis: TVec3): TMatrix4;
var
  SecondAxis, ThirdAxis: TVec3;
begin
  SecondAxis := VecAxis.RightAngled().Normal();
  ThirdAxis := VecAxis.Cross(SecondAxis);
  Result := ReorientationMatrix(ThirdAxis, SecondAxis, VecAxis);
end;

function ReflectionMatrix(const VecAxis: TVec3): TMatrix4;
const
  SQRT2 = Sqrt(2);
var
  XX, YY, ZZ, XY, XZ, YZ: GLfloat;
begin
  with (VecAxis * SQRT2) do begin
    XX := X * X;  YY := Y * Y;  ZZ := Z * Z;
    XY := X * Y;  XZ := X * Z;  YZ := Y * Z;
  end;
  Result := Matrix4([
    1 - XX,      -XY,      -XZ,   0.0,
       -XY,   1 - YY,      -YZ,   0.0,
       -XZ,      -YZ,   1 - ZZ,   0.0,
       0.0,      0.0,      0.0,   1.0
  ]);
end;

function TransformationMatrix(const Operations: array of TMatrix4): TMatrix4;
var
  I: Integer;
begin
  if Length(Operations) = 0 then begin
    Result := MATRIX4_IDENTITY;
    Exit();
  end;
  Result := Operations[Length(Operations) - 1];
  I := Length(Operations) - 2;
  while I >= 0 do begin
    Result *= Operations[I];
    I -= 1;
  end;
end;

operator =(const MatA, MatB: TMatrix4): Boolean;
begin
  Result := CompareByte(MatA, MatB, SizeOf(MatA)) = 0;
end;

operator +(const MatA, MatB: TMatrix4): TMatrix4;
var
  I, J: Integer;
begin
  for J := 0 to 3 do begin
    for I := 0 to 3 do Result[I, J] := MatA[I, J] + MatB[I, J];
  end;
end;

operator -(const MatA, MatB: TMatrix4): TMatrix4;
var
  I, J: Integer;
begin
  for J := 0 to 3 do begin
    for I := 0 to 3 do Result[I, J] := MatA[I, J] - MatB[I, J];
  end;
end;

operator *(const MatA, MatB: TMatrix4): TMatrix4;
var
  I, J, K: Integer;
begin
  for J := 0 to 3 do begin
    for I := 0 to 3 do begin
      Result[I, J] := 0.0;
      for K := 0 to 3 do Result[I, J] := Result[I, J] + MatA[K, J] * MatB[I, K];
    end;
  end;
end;

operator *(const MatA: TMatrix4; FacB: GLfloat): TMatrix4;
var
  I, J: Integer;
begin
  for J := 0 to 3 do begin
    for I := 0 to 3 do Result[I, J] := MatA[I, J] * FacB;
  end;
end;

operator *(const MatA: TMatrix4; const VecB: TVec3): TVec3;
begin
  Result := Vec3(
    MatA[0, 0] * VecB.X + MatA[1, 0] * VecB.Y + MatA[2, 0] * VecB.Z + MatA[3, 0],
    MatA[0, 1] * VecB.X + MatA[1, 1] * VecB.Y + MatA[2, 1] * VecB.Z + MatA[3, 1],
    MatA[0, 2] * VecB.X + MatA[1, 2] * VecB.Y + MatA[2, 2] * VecB.Z + MatA[3, 2]
  );
end;

operator *(const MatA: TMatrix4; const VecB: TVec4): TVec4;
begin
  Result := Vec4(
    MatA[0, 0] * VecB.X + MatA[1, 0] * VecB.Y + MatA[2, 0] * VecB.Z + MatA[3, 0] * VecB.W,
    MatA[0, 1] * VecB.X + MatA[1, 1] * VecB.Y + MatA[2, 1] * VecB.Z + MatA[3, 1] * VecB.W,
    MatA[0, 2] * VecB.X + MatA[1, 2] * VecB.Y + MatA[2, 2] * VecB.Z + MatA[3, 2] * VecB.W,
    MatA[0, 3] * VecB.X + MatA[1, 3] * VecB.Y + MatA[2, 3] * VecB.Z + MatA[3, 3] * VecB.W
  );
end;

function TMatrix4Helper.Inverse(): TMatrix4;
var
  A2323, A1323, A1223, A0323, A0223, A0123, A2313, A1313, A1213,
  A2312, A1312, A1212, A0313, A0213, A0312, A0212, A0113, A0112: GLfloat;
  Determinant: GLfloat;
begin
  A2323 := Self[2, 2] * Self[3, 3] - Self[2, 3] * Self[3, 2];
  A1323 := Self[2, 1] * Self[3, 3] - Self[2, 3] * Self[3, 1];
  A1223 := Self[2, 1] * Self[3, 2] - Self[2, 2] * Self[3, 1];
  A0323 := Self[2, 0] * Self[3, 3] - Self[2, 3] * Self[3, 0];
  A0223 := Self[2, 0] * Self[3, 2] - Self[2, 2] * Self[3, 0];
  A0123 := Self[2, 0] * Self[3, 1] - Self[2, 1] * Self[3, 0];

  Determinant := Self[0, 0] * (Self[1, 1] * A2323 - Self[1, 2] * A1323 + Self[1, 3] * A1223)
               - Self[0, 1] * (Self[1, 0] * A2323 - Self[1, 2] * A0323 + Self[1, 3] * A0223)
               + Self[0, 2] * (Self[1, 0] * A1323 - Self[1, 1] * A0323 + Self[1, 3] * A0123)
               - Self[0, 3] * (Self[1, 0] * A1223 - Self[1, 1] * A0223 + Self[1, 2] * A0123);
  if Determinant = 0.0 then Exit(Self);
  Determinant := 1.0 / Determinant;

  A2313 := Self[1, 2] * Self[3, 3] - Self[1, 3] * Self[3, 2];
  A1313 := Self[1, 1] * Self[3, 3] - Self[1, 3] * Self[3, 1];
  A1213 := Self[1, 1] * Self[3, 2] - Self[1, 2] * Self[3, 1];
  A2312 := Self[1, 2] * Self[2, 3] - Self[1, 3] * Self[2, 2];
  A1312 := Self[1, 1] * Self[2, 3] - Self[1, 3] * Self[2, 1];
  A1212 := Self[1, 1] * Self[2, 2] - Self[1, 2] * Self[2, 1];
  A0313 := Self[1, 0] * Self[3, 3] - Self[1, 3] * Self[3, 0];
  A0213 := Self[1, 0] * Self[3, 2] - Self[1, 2] * Self[3, 0];
  A0312 := Self[1, 0] * Self[2, 3] - Self[1, 3] * Self[2, 0];
  A0212 := Self[1, 0] * Self[2, 2] - Self[1, 2] * Self[2, 0];
  A0113 := Self[1, 0] * Self[3, 1] - Self[1, 1] * Self[3, 0];
  A0112 := Self[1, 0] * Self[2, 1] - Self[1, 1] * Self[2, 0];

  Result[0, 0] :=  Determinant * (Self[1, 1] * A2323 - Self[1, 2] * A1323 + Self[1, 3] * A1223);
  Result[0, 1] := -Determinant * (Self[0, 1] * A2323 - Self[0, 2] * A1323 + Self[0, 3] * A1223);
  Result[0, 2] :=  Determinant * (Self[0, 1] * A2313 - Self[0, 2] * A1313 + Self[0, 3] * A1213);
  Result[0, 3] := -Determinant * (Self[0, 1] * A2312 - Self[0, 2] * A1312 + Self[0, 3] * A1212);
  Result[1, 0] := -Determinant * (Self[1, 0] * A2323 - Self[1, 2] * A0323 + Self[1, 3] * A0223);
  Result[1, 1] :=  Determinant * (Self[0, 0] * A2323 - Self[0, 2] * A0323 + Self[0, 3] * A0223);
  Result[1, 2] := -Determinant * (Self[0, 0] * A2313 - Self[0, 2] * A0313 + Self[0, 3] * A0213);
  Result[1, 3] :=  Determinant * (Self[0, 0] * A2312 - Self[0, 2] * A0312 + Self[0, 3] * A0212);
  Result[2, 0] :=  Determinant * (Self[1, 0] * A1323 - Self[1, 1] * A0323 + Self[1, 3] * A0123);
  Result[2, 1] := -Determinant * (Self[0, 0] * A1323 - Self[0, 1] * A0323 + Self[0, 3] * A0123);
  Result[2, 2] :=  Determinant * (Self[0, 0] * A1313 - Self[0, 1] * A0313 + Self[0, 3] * A0113);
  Result[2, 3] := -Determinant * (Self[0, 0] * A1312 - Self[0, 1] * A0312 + Self[0, 3] * A0112);
  Result[3, 0] := -Determinant * (Self[1, 0] * A1223 - Self[1, 1] * A0223 + Self[1, 2] * A0123);
  Result[3, 1] :=  Determinant * (Self[0, 0] * A1223 - Self[0, 1] * A0223 + Self[0, 2] * A0123);
  Result[3, 2] := -Determinant * (Self[0, 0] * A1213 - Self[0, 1] * A0213 + Self[0, 2] * A0113);
  Result[3, 3] :=  Determinant * (Self[0, 0] * A1212 - Self[0, 1] * A0212 + Self[0, 2] * A0112);
end;

function TMatrix4Helper.Inverse3x3(): TMatrix4;
var
  A1221, A1220, A1210: GLfloat;
  Determinant: GLfloat;
begin
  A1221 := Self[1, 1] * Self[2, 2] - Self[2, 1] * Self[1, 2];
  A1220 := Self[0, 1] * Self[2, 2] - Self[2, 1] * Self[0, 2];
  A1210 := Self[0, 1] * Self[1, 2] - Self[1, 1] * Self[0, 2];

  Determinant := Self[0, 0] * A1221 - Self[1, 0] * A1220 + Self[2, 0] * A1210;
  if Determinant = 0.0 then Exit(Self);
  Determinant := 1.0 / Determinant;

  Result[0, 0] :=  Determinant * A1221;
  Result[0, 1] := -Determinant * A1220;
  Result[0, 2] :=  Determinant * A1210;
  Result[1, 0] := -Determinant * (Self[1, 0] * Self[2, 2] - Self[2, 0] * Self[1, 2]);
  Result[1, 1] :=  Determinant * (Self[0, 0] * Self[2, 2] - Self[2, 0] * Self[0, 2]);
  Result[1, 2] := -Determinant * (Self[0, 0] * Self[1, 2] - Self[1, 0] * Self[0, 2]);
  Result[2, 0] :=  Determinant * (Self[1, 0] * Self[2, 1] - Self[2, 0] * Self[1, 1]);
  Result[2, 1] := -Determinant * (Self[0, 0] * Self[2, 1] - Self[2, 0] * Self[0, 1]);
  Result[2, 2] :=  Determinant * (Self[0, 0] * Self[1, 1] - Self[1, 0] * Self[0, 1]);

  Result[0, 3] := Self[0, 3];   Result[1, 3] := Self[1, 3];   Result[2, 3] := Self[2, 3];
  Result[3, 0] := Self[3, 0];   Result[3, 1] := Self[3, 1];   Result[3, 2] := Self[3, 2];
  Result[3, 3] := Self[3, 3];
end;

function TMatrix4Helper.Transpose(): TMatrix4;
var
  I, J: Integer;
begin
  for J := 0 to 3 do begin
    for I := 0 to 3 do Result[I, J] := Self[J, I];
  end;
end;

function TMatrix4Helper.ToString(): String;
var
  I: Integer;
begin
  Result := 'TMatrix4 {' + LineEnding;
  for I := 0 to 3 do begin
    Result += Format('  [ %.5f | %.5f | %.5f | %.5f ]',
      [Self[0][I], Self[1][I], Self[2][I], Self[3][I]],
      VOX_FORMAT
    ) + LineEnding;
  end;
  Result += '}';
end;

// #########################################################
// ###                     Utilities                     ###
// #########################################################

procedure PrintElementBuffer(Elements: array of GLuint; Stride: Integer = 3; LineEndingStride: Integer = -1);
var
  BlockCount: Integer;
  I, J: Integer;
begin
  BlockCount := Length(Elements) div Stride;
  for I := 0 to BlockCount - 1 do begin
    WriteF('[%u](', [I], False);
    for J := 0 to Stride - 1 do begin
      Write(Elements[J + I * Stride]);
      if J <> Stride - 1 then Write(', ');
    end;
    Write(')');
    if I <> BlockCount - 1 then begin
      Write(', ');
      if (LineEndingStride > 0) and ((I + 1) mod LineEndingStride = 0) and (I > 0) then WriteLn('');
    end;
  end;
  WriteLn('');
end;

procedure WriteF(const Str: String; const Values: array of const; AddNewLine: Boolean = True);
begin
  Write(Format(Str, Values));
  if AddNewLine then Write(LineEnding);
end;

{$endif}

{initialization}
{$ifdef INCLUDE_SECTION_INITIALIZATION}

begin
  VOX_FORMAT := DefaultFormatSettings;
  VOX_FORMAT.DecimalSeparator := '.';
end;

{$endif}
