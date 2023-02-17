#include "engine.h"

#define MAX_ARRAY_LENGTH 4096
#define MAX_BOMBS 9
#define X_TILES 10
#define Y_TILES 10

enum {EMPTY, NUMBER, BOMB};

// Types

typedef struct { 
    v3 Position;
    mesh Mesh;
    color Color;
    int Type;
    int Hit;
    int Visible;
    int Flagged;
    int BombsNearAmount;
} entity;

typedef struct {
    entity* Items;
    int Length;
    int Capacity;
    int Index;
} entityArray;

typedef struct {
    v3 Items[X_TILES * Y_TILES];
    int First;
    int Index;
    int Length;
} queue;

typedef struct {
    v3 Items[8];
    int Length;
} neighbors;

// Globals

entityArray Entities;
timer Timer;
grid Grid;

int Flags = MAX_BOMBS;
int Playing = 1;
int FirstPick;
int Win;

// colors

color ColorEmpty = {0.1f, 0.1f, 0.1f, 1.0f};
color ColorHidden = {0.02f, 0.02f, 0.02f, 1.0f};
color ColorBomb = {0.1f, 0.1f, 0.1f, 1.0f};
color ColorFlag = {0.2f, 0.2f, 0.2f, 1.0f};
color ColorBombHit = {0.8f, 0.1f, 0.1f, 1.0f};
color ColorText = {0.1f, 0.1f, 0.1f, 1.0f};
color ColorTextGameOver = {0.8f, 0.1f, 0.1f, 1.0f};
color ColorTextWin = {0.1f, 0.8f, 0.1f, 1.0f};
color Color1 = {0.3f, 0.1f, 0.8f, 1.0f};
color Color2 = {0.1f, 0.6f, 0.1f, 1.0f};
color Color3 = {0.6f, 0.6f, 0.1f, 1.0f};
color Color4 = {0.6f, 0.2f, 0.6f, 1.0f};
color Color5 = {0.6f, 0.1f, 0.1f, 1.0f};
color Color6 = {0.6f, 0.3f, 0.1f, 1.0f};
color Color7 = {0.7f, 0.1f, 0.1f, 1.0f};
color Color8 = {0.6f, 0.1f, 0.1f, 1.0f};

// Declarations

void AddEntityToArray(entity* Entity, entityArray* Array);
void DrawEntity(entity* Entity);
void ClearArray(entityArray* Array);
void QueueAdd(queue* Queue, v3 Position);
void FloodEmpty(v3 Start);
void RevealAll();
void CalculateNumbers();

neighbors GetNeighborsByType(v3 Position, int Type);
v3 QueuePop(queue* Queue);

int QueueHasItem(queue* Queue, v3 Position);
int AddBomb();

neighbors GetNeighborsByType(v3 Position, int Type) {
    
    neighbors Neighbors = {0};
    int NeighborIndex = 0;
    
    int XYOffsets[] = {
        -1,-1, 0,-1, 1,-1,
        -1, 0,       1, 0,
        -1, 1, 0, 1, 1, 1,  
    };
    
    for(int Index = 0; Index < 16; Index += 2) {
        int XNeighbor = Position.X + XYOffsets[Index];
        int YNeighbor = Position.Y + XYOffsets[Index+1];
        if(XNeighbor < 0 || YNeighbor < 0 || XNeighbor >= X_TILES || YNeighbor >= Y_TILES || (Entities.Items[YNeighbor * X_TILES + XNeighbor].Type != Type)) {
            continue;
        }
        Neighbors.Items[NeighborIndex++] = (v3){XNeighbor, YNeighbor, 0.0f};
        ++Neighbors.Length;
    }
    
    return Neighbors;
}

void QueueAdd(queue* Queue, v3 Position) {
    ++Queue->Length;
    Queue->Items[Queue->Index++] = Position;
}

v3 QueuePop(queue* Queue) {
    --Queue->Length;
    return Queue->Items[Queue->First++];
}

int QueueHasItem(queue* Queue, v3 Position) {
    for(int Index = 0; Index < Queue->Length; ++Index) {
        if(V3Compare(Queue->Items[Index], Position)) {
            return 1;
        }
    }
    return 0;
}

void FloodEmpty(v3 Start) {
    
    queue Frontier = {0};
    queue Reached = {0};
    
    QueueAdd(&Frontier, Start);
    QueueAdd(&Reached, Start);
    
    // Flood from Start and make visible
    
    while(Frontier.Length > 0) {
        v3 Current = QueuePop(&Frontier);
        neighbors Neighbors = GetNeighborsByType(Current, EMPTY);
        
        for(int Index = 0; Index < Neighbors.Length; ++Index) {
            if(!QueueHasItem(&Reached, Neighbors.Items[Index])) {
                QueueAdd(&Reached, Neighbors.Items[Index]);
                QueueAdd(&Frontier, Neighbors.Items[Index]);
                
                int X = Neighbors.Items[Index].X;
                int Y = Neighbors.Items[Index].Y;
                Entities.Items[Y * X_TILES + X].Visible = 1;
                
                // Reveal numbers around the tile
                
                neighbors NumberNeighbors = GetNeighborsByType(Entities.Items[Y * X_TILES + X].Position, NUMBER);
                for(int Index = 0; Index < NumberNeighbors.Length; ++Index) {
                    int X = NumberNeighbors.Items[Index].X;
                    int Y = NumberNeighbors.Items[Index].Y;
                    Entities.Items[Y * X_TILES + X].Visible = 1;
                }
            }
        }
    }
    
}

entityArray NewEntityArray(int Capacity) {
    entityArray Array = {
        .Items = MemoryAlloc(Capacity * sizeof(entity)),
        .Capacity = Capacity,
    };
    return Array;
}

// Overrides elements starting from index 0 if overflowing

void AddEntityToArray(entity* Entity, entityArray* Array) {
    Array->Items[Array->Index++] = *Entity;
    if(Array->Length < Array->Capacity) {
        ++Array->Length;
    }
    if(Array->Index >= Array->Capacity) {
        Array->Index = 0;
    }
}

void RevealAll() {
    for(int Index = 0; Index < Entities.Length; ++Index) {
        Entities.Items[Index].Visible = 1;
    }
}

void DrawEntity(entity* Entity) {
    
    float UVSize = 1.0f / 16.0f;
    float UOffset = 2 * UVSize;
    float VOffset = 11 * UVSize;
    color Color = ColorHidden;
    
    if(Entity->Visible) {
        switch(Entity->Type) {
            case NUMBER: {
                char Char = '0' + Entity->BombsNearAmount;
                UOffset = Char % 16 * UVSize;
                VOffset = Char / 16 * UVSize;
                switch(Entity->BombsNearAmount) {
                    case 1: { Color = Color1; } break; 
                    case 2: { Color = Color2; } break; 
                    case 3: { Color = Color3; } break; 
                    case 4: { Color = Color4; } break; 
                    case 5: { Color = Color5; } break; 
                    case 6: { Color = Color6; } break; 
                    case 7: { Color = Color7; } break; 
                    case 8: { Color = Color8; } break; 
                }
            } break;
            case BOMB: {
                UOffset = 15 * UVSize;
                VOffset = 0 * UVSize;
                Color = ColorBomb;
            } break;
            case EMPTY: {
                UOffset = 9 * UVSize;
                VOffset = 15 * UVSize;
                Color = ColorEmpty;
            } break;
        }
    }
    
    if(Entity->Flagged) {
        UOffset = 11 * UVSize;
        VOffset = 15 * UVSize;
        Color = ColorFlag;
    }
    
    if(!Playing && Entity->Hit) {
        Color = ColorBombHit;
    }
    
    DrawOne(Entity->Position, 
            Color, 
            Entity->Mesh, 
            UOffset, 
            VOffset);
}

void CalculateNumbers() {
    
    for(int Y = 0; Y < Y_TILES; ++Y) {
        for(int X = 0; X < X_TILES; ++X) {
            
            entity* Entity = &Entities.Items[Y * X_TILES + X];
            
            if(Entity->Type == BOMB) continue;
            
            Entity->BombsNearAmount = 0;
            Entity->Type = EMPTY;
            
            neighbors Neighbors = GetNeighborsByType(Entity->Position, BOMB);
            
            if(Neighbors.Length > 0) {
                Entity->Type = NUMBER;
                Entity->BombsNearAmount = Neighbors.Length;
            }
        }
    }
}

int AddBomb(v3* Position) {
    int X = rand() % X_TILES;
    int Y = rand() % Y_TILES;
    
    // Exclude position if not NULL
    if(Position != NULL &&
       (float)X == Position->X &&
       (float)Y == Position->Y) {
        return 0;
    }
    
    if(Entities.Items[Y * X_TILES + X].Type == EMPTY) {
        Entities.Items[Y * X_TILES + X].Type = BOMB;
        return 1;
    }
    return 0;
}

void Init() {
    
    Entities = NewEntityArray(X_TILES * Y_TILES);
    
    Grid = (grid){ 
        .Width = 10, 
        .Height = 10,
        .Color = ColorGrid,
    };
    
    GridInit(&Grid);
    
    // Reset things when starting a new game
    
    FirstPick = 0;
    Playing = 1;
    Win = 0;
    Flags = MAX_BOMBS;
    InitTimer(&Timer);
    Mouse.LeftButtonPressed = 0;
    Mouse.RightButtonPressed = 0;
    
    // Empty tiles
    
    for(int Y = 0; Y < Y_TILES; ++Y) {
        for(int X = 0; X < X_TILES; ++X) {
            entity Entity = {
                .Position = {X, Y},
                .Color = ColorHidden,
                .Mesh = MeshRectangle,
                .Type = EMPTY,
                .Visible = 0,
            };
            AddEntityToArray(&Entity, &Entities);
        }
    }
    
    // Bombs
    
    int Bombs = 0;
    while((Bombs += AddBomb(NULL)) < MAX_BOMBS);
    
    // Numbers
    
    CalculateNumbers();
    
}

void Input() {
    
    // Reset
    
    if(KeyPressed[SPACE]) {
        Init();
        KeyPressed[SPACE] = 0;
    }
    
    if(!Playing) return;
    
    // Pick
    
    if(Mouse.LeftButtonPressed) {
        
        if(FirstPick == 0) FirstPick = 1;
        
        Mouse.LeftButtonPressed = 0;
        
        for(int Index = 0; Index < Entities.Length; ++Index) {
            
            entity* Entity = &Entities.Items[Index];
            
            if(Entity->Flagged) continue;
            
            if(PickMeshRectangle(Mouse.X, Mouse.Y, Entity->Position, &Entity->Mesh)) {
                Entity->Visible = 1;
                
                if(Entity->Type == EMPTY) {
                    FloodEmpty(Entity->Position);
                } else if(Entity->Type == BOMB) {
                    // Relocate bomb if hit with first pick
                    if(FirstPick == 1) {
                        Entity->Type = EMPTY;
                        while(AddBomb(&Entity->Position) == 0);
                        CalculateNumbers();
                        FloodEmpty(Entity->Position);
                    } else {
                        Playing = 0;
                        Win = 0;
                        RevealAll();
                        Entity->Hit = 1;
                    }
                }
            }
        }
        
        // Check win condition
        
        if(Playing) {
            
            int End = 1;
            
            // If any non bomb is hidden, the game continues
            for(int Index = 0; Index < Entities.Length; ++Index) {
                entity* Entity = &Entities.Items[Index];
                if(!Entity->Visible && Entity->Type != BOMB) {
                    End = 0;
                    break;
                }
            }
            if(End) {
                Playing = 0;
                Win = 1;
                RevealAll();
            }
        }
        
        FirstPick = 2;
    }
    
    if(Mouse.RightButtonPressed) {
        
        Mouse.RightButtonPressed = 0;
        
        for(int Index = 0; Index < Entities.Length; ++Index) {
            
            entity* Entity = &Entities.Items[Index];
            
            if(PickMeshRectangle(Mouse.X, Mouse.Y, Entity->Position, &Entity->Mesh)) {
                
                if(Entity->Flagged) {
                    Entity->Flagged = 0;
                    ++Flags;
                } else {
                    if(Flags > 0) {
                        Entity->Flagged = 1;
                        --Flags;
                    }
                }
            }
        }
    }
    
    // Camera
    
    v3 CameraAcceleration = {0};
    
    if(KeyDown[W]) {
        CameraAcceleration.Y = 1.0f; 
    }
    if(KeyDown[A]) {
        CameraAcceleration.X = -1.0f; 
    }
    if(KeyDown[S]) {
        CameraAcceleration.Y = -1.0f; 
    }
    if(KeyDown[D]) {
        CameraAcceleration.X = 1.0f; 
    }
    if(KeyDown[Q]) {
        CameraAcceleration.Z = -1.0f;
    }
    if(KeyDown[E]) {
        CameraAcceleration.Z = 1.0f; 
    }
    
    if(Mouse.WheelDown) {
        CameraAcceleration.Z = -10.0f;
        Mouse.WheelDown = 0;
    }
    if(Mouse.WheelUp) {
        CameraAcceleration.Z = 10.0f;
        Mouse.WheelUp = 0;
    }
    
    CameraUpdateByAcceleration(CameraAcceleration);
    
}

void Update() {
    if(!Playing) return;
    UpdateTimer(&Timer); 
};

void Draw() {
    
    // Entities
    
    for(int Index = 0; Index < Entities.Length; ++Index) {
        entity* Entity = &Entities.Items[Index];
        DrawEntity(Entity);
    }
    
    // Texts
    
    char* TimerText = MemoryAlloc(32 * sizeof(*TimerText));
    sprintf(TimerText, "%3d", (int)(Timer.ElapsedMilliSeconds / 1000.0f));
    DrawString(
               (v3){7.0f, 10.0f, 0.0f},
               TimerText,
               ColorText
               );
    
    
    char* FlagsText = MemoryAlloc(32 * sizeof(*FlagsText));
    sprintf(FlagsText, "%2d", Flags);
    
    DrawString(
               (v3){0.0f, 10.0f, 0.0f},
               FlagsText,
               ColorText
               );
    
    
    if(!Playing) {
        if(Win) {
            DrawString(
                       (v3){0.0f, -1.0f, 0.0f},
                       "You win!",
                       ColorTextWin
                       );
        } else {
            DrawString(
                       (v3){0.0f, -1.0f, 0.0f},
                       "Game over!",
                       ColorTextGameOver
                       );
        }
        
    }
    
    GridDraw(&Grid);
}
