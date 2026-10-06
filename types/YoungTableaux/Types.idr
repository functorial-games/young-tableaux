module YoungTableaux.Types

import Data.Vect

%default total

-- Design sketch: intentionally broader than the first APK.
-- The purpose is to enumerate mathematical inputs, operations, interaction
-- events, controls, conventions, and outputs without forcing all of them
-- into the first implementation.

public export
partitionRowsValid : List Nat -> Bool
partitionRowsValid [] = True
partitionRowsValid [row] = row > 0
partitionRowsValid (row :: next :: rest) = row >= next && partitionRowsValid (next :: rest)

public export
record Partition where
  constructor MkPartition
  rows : List Nat
  validRows : partitionRowsValid rows = True

public export
record Cell where
  constructor MkCell
  row : Nat
  column : Nat

-- Direct corner edits follow edges in the Young graph, not merely mutations of
-- the final row lengths.  Keeping the additions makes undo/reset and later
-- path-based constructions explicit.
public export
record DiagramState where
  constructor MkDiagramState
  base : Partition
  current : Partition
  additions : List Cell

public export
data DiagramEvent
  = ClickAddable Cell
  | ClickRemovable Cell
  | UndoLastAddition
  | ResetAdditions
  | ReplacePartition Partition

public export
record SkewShape where
  constructor MkSkewShape
  outer : Partition
  inner : Partition

-- Native skew_tableau_parse additionally checks inner containment. A slide
-- hole is execution state, never a TableauKind or an extra filled cell.

public export
data TableauKind
  = ArbitraryFilling
  | RowStandard
  | ColumnStandard
  | Standard
  | Semistandard
  | Skew
  | Shifted
  | Ribbon
  | Oscillating
  | KTableau

public export
record Entry where
  constructor MkEntry
  value : Integer

public export
record FilledCell where
  constructor MkFilledCell
  position : Cell
  entry : Entry

public export
record Tableau where
  constructor MkTableau
  kind : TableauKind
  shape : SkewShape
  cells : List FilledCell

-- These are the straight final RSK families. Native constructors additionally
-- check the actual alphabet and row/column inequalities, not only the tag.
public export
record StandardTableau where
  constructor MkStandardTableau
  filling : Tableau
  standardKind : kind filling = Standard
  straightShape : rows (inner (shape filling)) = []

public export
record SemistandardTableau where
  constructor MkSemistandardTableau
  filling : Tableau
  semistandardKind : kind filling = Semistandard
  straightShape : rows (inner (shape filling)) = []

public export
record PermutationRSKResult where
  constructor MkPermutationRSKResult
  insertion : StandardTableau
  recording : StandardTableau
  sameShape : shape (filling insertion) = shape (filling recording)

public export
record WordRSKResult where
  constructor MkWordRSKResult
  insertion : SemistandardTableau
  recording : StandardTableau
  sameShape : shape (filling insertion) = shape (filling recording)

public export
record BiwordRSKResult where
  constructor MkBiwordRSKResult
  insertion : SemistandardTableau
  recording : SemistandardTableau
  sameShape : shape (filling insertion) = shape (filling recording)

-- A filling and an in-progress hole belong to different executable types.
public export
record SkewTableau where
  constructor MkSkewTableau
  shape : SkewShape
  cells : List FilledCell

public export
data JeuPhase = CompleteFilling | SlidingHole Cell

public export
record JeuState where
  constructor MkJeuState
  filling : SkewTableau
  phase : JeuPhase

public export
occurrences : Nat -> List Nat -> Nat
occurrences value [] = 0
occurrences value (first :: rest) = (if value == first then 1 else 0) + occurrences value rest

public export
permutationValid : List Nat -> Bool
permutationValid values = all (\value => value > 0 && value <= length values && occurrences value values == 1) values

public export
record Permutation where
  constructor MkPermutation
  values : List Nat
  validPermutation : permutationValid values = True

public export
record Word where
  constructor MkWord
  letters : List Integer
  positiveLetters : all (> 0) letters = True

public export
record Biword where
  constructor MkBiword
  count : Nat
  letters : Vect count (Integer, Integer)
  -- Native Biword is a list of paired Biletter values. rsk_biword checks
  -- positive letters and lexicographic order for row insertion, including
  -- nondecreasing bottom letters when top letters are equal.

public export
record NatMatrix where
  constructor MkNatMatrix
  height : Nat
  width : Nat
  entries : Vect height (Vect width Nat)

public export
data Characteristic
  = CharacteristicZero
  | CharacteristicP Nat

public export
data GroupElement
  = PermutationElement Permutation
  | CycleType Partition

public export
data SymmetricFunctionBasis
  = Monomial
  | Elementary
  | CompleteHomogeneous
  | PowerSum
  | Schur
  | HallLittlewood
  | Macdonald

public export
record SymmetricFunctionTerm where
  constructor MkSymmetricFunctionTerm
  coefficient : Integer
  index : Partition

public export
record SymmetricFunction where
  constructor MkSymmetricFunction
  basis : SymmetricFunctionBasis
  terms : List SymmetricFunctionTerm

public export
record Specialization where
  constructor MkSpecialization
  values : List (String, Integer)

public export
record PolynomialTerm where
  constructor MkPolynomialTerm
  coefficient : Integer
  powers : List Nat

public export
record Polynomial where
  constructor MkPolynomial
  variables : List String
  terms : List PolynomialTerm

-- Conventions are explicit inputs, not buried implementation assumptions.

public export
data DiagramOrientation
  = English
  | French

public export
data IndexOrigin
  = ZeroBased
  | OneBased

public export
data ContentConvention
  = ColumnMinusRow
  | RowMinusColumn

public export
data InsertionConvention
  = RowInsertion
  | ColumnInsertion

public export
data ReadingOrder
  = RowReading
  | ReverseRowReading
  | ColumnReading
  | ReverseColumnReading

public export
data ActionSide
  = LeftAction
  | RightAction

public export
record Conventions where
  constructor MkConventions
  orientation : DiagramOrientation
  indexOrigin : IndexOrigin
  contentConvention : ContentConvention
  insertionConvention : InsertionConvention
  readingOrder : ReadingOrder
  actionSide : ActionSide

-- Mathematical operations. More can be added without changing the renderer.

public export
data Operation
  = DisplayPartition
  | ListCells
  | DisplayTableau
  | ConjugatePartition
  | ComputeHookLengths
  | ComputeHookProduct
  | CountStandardTableaux
  | FindCorners
  | FindAddableCells
  | FindRemovableCells
  | AddCell
  | RemoveCell
  | ValidateTableau
  | StandardizeTableau
  | InsertLetter
  | ReverseInsert
  | JeuDeTaquinSlide
  | Rectify
  | Promote
  | Evacuate
  | TransposeTableau
  | RSKPermutation
  | RSKWord
  | RSKBiword
  | RSKMatrix
  | InverseRSK
  | LittlewoodRichardsonCoefficient
  | EnumerateLRTableaux
  | MultiplySchurFunctions
  | ChangeBasis
  | Specialize
  | Plethysm
  | BranchUp
  | BranchDown
  | EnumerateYoungGraphPaths
  | CharacterValue
  | RepresentationDimension
  | GenerateRandomPermutation
  | GenerateRandomStandardTableau
  | SamplePlancherelPartition
  | ComputeLongestIncreasingSubsequence
  | ComputeLongestDecreasingSubsequence
  | CoxeterReducedWord
  | BruhatRelations

public export
InputFor : Operation -> Type
InputFor DisplayPartition = Partition
InputFor ListCells = Partition
InputFor DisplayTableau = Tableau
InputFor ConjugatePartition = Partition
InputFor ComputeHookLengths = Partition
InputFor ComputeHookProduct = Partition
InputFor CountStandardTableaux = Partition
InputFor FindCorners = Partition
InputFor FindAddableCells = Partition
InputFor FindRemovableCells = Partition
InputFor AddCell = (Partition, Cell)
InputFor RemoveCell = (Partition, Cell)
InputFor ValidateTableau = Tableau
InputFor StandardizeTableau = Tableau
InputFor InsertLetter = (Tableau, Entry)
InputFor ReverseInsert = (Tableau, Cell)
InputFor JeuDeTaquinSlide = (SkewTableau, Cell)
InputFor Rectify = SkewTableau
InputFor Promote = Tableau
InputFor Evacuate = Tableau
InputFor TransposeTableau = Tableau
InputFor RSKPermutation = Permutation
InputFor RSKWord = Word
InputFor RSKBiword = Biword
InputFor RSKMatrix = NatMatrix
InputFor InverseRSK = PermutationRSKResult
InputFor LittlewoodRichardsonCoefficient = (Partition, Partition, Partition)
InputFor EnumerateLRTableaux = (Partition, Partition, Partition)
InputFor MultiplySchurFunctions = (Partition, Partition)
InputFor ChangeBasis = (SymmetricFunction, SymmetricFunctionBasis)
InputFor Specialize = (SymmetricFunction, Specialization)
InputFor Plethysm = (SymmetricFunction, SymmetricFunction)
InputFor BranchUp = Partition
InputFor BranchDown = Partition
InputFor EnumerateYoungGraphPaths = (Partition, Partition)
InputFor CharacterValue = (Partition, GroupElement)
InputFor RepresentationDimension = Partition
InputFor GenerateRandomPermutation = Nat
InputFor GenerateRandomStandardTableau = Partition
InputFor SamplePlancherelPartition = Nat
InputFor ComputeLongestIncreasingSubsequence = Permutation
InputFor ComputeLongestDecreasingSubsequence = Permutation
InputFor CoxeterReducedWord = Permutation
InputFor BruhatRelations = (Permutation, Permutation)

public export
data DiagramResult
  = Diagram Partition
  | TableauDiagram Tableau

public export
OutputFor : Operation -> Type
OutputFor DisplayPartition = DiagramResult
OutputFor ListCells = List Cell
OutputFor DisplayTableau = DiagramResult
OutputFor ConjugatePartition = Partition
OutputFor ComputeHookLengths = List (Cell, Nat)
OutputFor ComputeHookProduct = Integer
OutputFor CountStandardTableaux = Integer
OutputFor FindCorners = List Cell
OutputFor FindAddableCells = List Cell
OutputFor FindRemovableCells = List Cell
OutputFor AddCell = Partition
OutputFor RemoveCell = Partition
OutputFor ValidateTableau = Bool
OutputFor StandardizeTableau = Tableau
OutputFor InsertLetter = Tableau
OutputFor ReverseInsert = Tableau
OutputFor JeuDeTaquinSlide = SkewTableau
OutputFor Rectify = SkewTableau
OutputFor Promote = Tableau
OutputFor Evacuate = Tableau
OutputFor TransposeTableau = Tableau
OutputFor RSKPermutation = PermutationRSKResult
OutputFor RSKWord = WordRSKResult
OutputFor RSKBiword = BiwordRSKResult
OutputFor RSKMatrix = BiwordRSKResult
OutputFor InverseRSK = Permutation
OutputFor LittlewoodRichardsonCoefficient = Nat
OutputFor EnumerateLRTableaux = List Tableau
OutputFor MultiplySchurFunctions = SymmetricFunction
OutputFor ChangeBasis = SymmetricFunction
OutputFor Specialize = Polynomial
OutputFor Plethysm = SymmetricFunction
OutputFor BranchUp = List Partition
OutputFor BranchDown = List Partition
OutputFor EnumerateYoungGraphPaths = List (List Partition)
OutputFor CharacterValue = Integer
OutputFor RepresentationDimension = Integer
OutputFor GenerateRandomPermutation = Permutation
OutputFor GenerateRandomStandardTableau = Tableau
OutputFor SamplePlancherelPartition = Partition
OutputFor ComputeLongestIncreasingSubsequence = List Nat
OutputFor ComputeLongestDecreasingSubsequence = List Nat
OutputFor CoxeterReducedWord = List Nat
OutputFor BruhatRelations = Bool

public export
record Request (operation : Operation) where
  constructor MkRequest
  conventions : Conventions
  input : InputFor operation

-- Raw interaction inputs. Keep the vocabulary larger than v0.1.

public export
record ScreenPoint where
  constructor MkScreenPoint
  x : Double
  y : Double

public export
data PointerButton
  = Primary
  | Secondary
  | Middle
  | ExtraButton Nat

public export
data TouchEvent
  = TouchDown Nat ScreenPoint
  | TouchMove Nat ScreenPoint
  | TouchUp Nat ScreenPoint
  | TouchCancel Nat

public export
data MouseEvent
  = MouseMove ScreenPoint
  | MouseDown PointerButton ScreenPoint
  | MouseUp PointerButton ScreenPoint
  | MouseWheel Double Double

public export
data Gesture
  = Tap ScreenPoint
  | DoubleTap ScreenPoint
  | LongPress ScreenPoint
  | Drag ScreenPoint ScreenPoint
  | Swipe ScreenPoint ScreenPoint
  | Pinch Double
  | RotateGesture Double
  | TwoFingerPan ScreenPoint ScreenPoint

public export
data Modifier
  = Shift
  | Control
  | Alt
  | Meta

public export
data Key
  = Character Char
  | Enter
  | Escape
  | Backspace
  | Delete
  | Tab
  | ArrowUp
  | ArrowDown
  | ArrowLeft
  | ArrowRight
  | Home
  | End
  | PageUp
  | PageDown
  | FunctionKey Nat

public export
data KeyEvent
  = KeyDown (List Modifier) Key
  | KeyUp (List Modifier) Key
  | TextInput String

public export
data GamepadButton
  = PadA
  | PadB
  | PadX
  | PadY
  | LeftBumper
  | RightBumper
  | LeftStickPress
  | RightStickPress
  | Start
  | Select
  | DPadUp
  | DPadDown
  | DPadLeft
  | DPadRight

public export
data GamepadAxis
  = LeftX
  | LeftY
  | RightX
  | RightY
  | LeftTrigger
  | RightTrigger

public export
data GamepadEvent
  = PadButtonDown GamepadButton
  | PadButtonUp GamepadButton
  | PadAxis GamepadAxis Double

public export
data Selection
  = SelectCell Cell
  | SelectPartition Partition
  | SelectTableau Tableau
  | SelectEntry Entry
  | SelectRow Nat
  | SelectColumn Nat
  | SelectCorner Cell
  | SelectPathStep Nat
  | SelectNothing

public export
data MathematicalAction
  = ChooseAddableCell Cell
  | ChooseRemovableCell Cell
  | EnterPartition Partition
  | EnterPermutation Permutation
  | EnterWord Word
  | EnterTableau Tableau
  | InsertValue Entry
  | RequestOperation Operation
  | Undo
  | Redo
  | Reset
  | Randomize

public export
data InteractionEvent
  = TouchInput TouchEvent
  | MouseInput MouseEvent
  | GestureInput Gesture
  | KeyboardInput KeyEvent
  | GamepadInput GamepadEvent
  | MathematicalInput MathematicalAction

public export
data ExternalInput
  = ClipboardText String
  | FileText String
  | URLInput String
  | QRPayload String
  | CommandLineArguments (List String)
  | SerializedState String
  | SharedAndroidIntent String

public export
data DeviceEvent
  = OrientationChanged Double Double Double
  | Accelerometer Double Double Double
  | WindowResized Nat Nat
  | AppPaused
  | AppResumed
  | LowMemory

public export
data AccessibilityAction
  = MoveFocusForward
  | MoveFocusBackward
  | ActivateFocused
  | IncreaseValue
  | DecreaseValue
  | ReadDescription

public export
data SystemCommand
  = SaveState
  | LoadState
  | Export
  | Import
  | Share
  | Quit

public export
data AppEvent
  = UserInteraction InteractionEvent
  | External ExternalInput
  | Device DeviceEvent
  | Accessibility AccessibilityAction
  | System SystemCommand

-- UI vocabulary: a small implementation vocabulary can expose a very large
-- mathematical vocabulary.

public export
data ControlType
  = TextField
  | IntegerField
  | IntegerStepper
  | RealField
  | Toggle
  | Choice
  | ActionButton
  | MatrixField
  | ListField
  | TableauField
  | Separator
  | Label
  | OutputText
  | OutputDiagram

public export
data ControlPurpose
  = SetN
  | SetLambda
  | SetMu
  | SetNu
  | SetPartitionRow Nat
  | SetSelectedCell
  | SetTableauEntries
  | SetTableauKind
  | SetAlphabet
  | SetEntry
  | SetWeight
  | SetPermutation
  | SetWord
  | SetBiword
  | SetMatrix
  | SetCharacteristic
  | SetPrimeCharacteristic
  | SetHeckeParameter
  | SetSymmetricFunctionBasis
  | SetQ
  | SetT
  | SetDiagramOrientation
  | SetContentConvention
  | SetInsertionConvention
  | SetReadingConvention
  | SetActionSide
  | Run Operation

public export
record ControlId where
  constructor MkControlId
  value : Nat

public export
record Control where
  constructor MkControl
  id : ControlId
  label : String
  purpose : ControlPurpose
  kind : ControlType
  enabled : Bool

-- Rendering stays deliberately primitive.

public export
data Mark
  = Block
  | Dot

public export
data RenderPrimitive
  = RPBlock
  | RPDot
  | RPLine
  | RPArrow
  | RPCircle
  | RPText
  | RPHighlight
  | RPBracket
  | RPEdge

public export
data CellDisplay
  = EmptyCell
  | EntryValue
  | HookLength
  | ContentValue
  | ArmLength
  | LegLength

public export
data HighlightMeaning
  = Selected
  | Addable
  | Removable
  | ActiveInsertion
  | JeuDeTaquinHole
  | Changed
  | Invalid
  | Source
  | Destination

public export
record DisplayOptions where
  constructor MkDisplayOptions
  primitive : RenderPrimitive
  cellDisplay : CellDisplay
  showGrid : Bool
  showCoordinates : Bool
  showCorners : Bool
  showAddable : Bool
  showRemovable : Bool
  showPath : Bool

-- The first APK should not assume one privileged object or operation.
-- It should expose a broad scrollable control surface and let the user learn
-- what inputs and outputs the various constructions require.
