/*
   SPOKE REMOTE



*/

#include "raylib.h"
#include "raymath.h"
// #include "screens.h"
#include "rlgl.h"

#include <string.h>             // Required for: strcpy()

#include <stdio.h>

#include <stdlib.h>

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "load_stl.h"

// raygui embedded styles
// #include "styles/style_cyber.h"       // raygui style: cyber
// #include "styles/style_jungle.h"      // raygui style: jungle
// #include "styles/style_lavanda.h"     // raygui style: lavanda
// #include "styles/style_default.h"     // raygui style: lavanda
// #include "styles/style_dark.h"        // raygui style: dark
// #include "styles/style_dark_large.h"  // raygui style: dark_large
// #include "styles/style_bluish.h"      // raygui style: bluish
// #include "styles/style_terminal.h"    // raygui style: terminal


#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION 330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION 330
#endif      

#define FLT_MAX     340282346638528859811704183484516925440.0f     // Maximum value of a float, from bit pattern 01111111011111111111111111111111

Font font;

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

#define LETTER_BOUNDRY_SIZE     0.25f
#define TEXT_MAX_LAYERS         32
#define LETTER_BOUNDRY_COLOR    VIOLET

bool SHOW_LETTER_BOUNDRY = false;
bool SHOW_TEXT_BOUNDRY = false;

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------
// Draw a codepoint in 3D space
static void DrawTextCodepoint3D(Font font, int codepoint, Vector3 position, float fontSize, bool backface, Color tint);
// Draw a 2D text in 3D space
static void DrawText3D(Font font, const char *text, Vector3 position, float fontSize, float fontSpacing, float lineSpacing, bool backface, Color tint);
// Measure a text in 3D. For some reason `MeasureTextEx()` just doesn't seem to work so i had to use this instead.
static Vector3 MeasureText3D(Font font, const char *text, float fontSize, float fontSpacing, float lineSpacing);

void DrawSplineBasis3D(Vector3 *points, int pointCount, Color color);
void DrawSplineSegmentBezierCubic3D(Vector3 p1, Vector3 c2, Vector3 c3, Vector3 p4, int segments, Color color, bool d );

int CubeInstanceCount;
Matrix *CubeInstances = 0;

Camera GameCamera = { 0 };
Mesh  GameCubeMesh;
Model GameCube;
Model GameSphere;
Model GameTorus;
Model GameCylinder;
Model GameCone;

Model SpokeObject;
BoundingBox SpokeObjectBoundingBox;

Shader GameShader;
int    ShaderAmbientLoc;

Shader FontShader;

Material MatInstances;
Shader InstancingShader;
int InstancingAmbientLoc;

Model GameModel;
BoundingBox GameModelBounds;

Model GameEsp32;

Mesh GameStlMesh;

Model GameStl;

// Ground quad
float GridSpacing = 10.0f;
int GridSlices = 20;
float GridSize = (GridSpacing * GridSlices / 2 );

Vector3 Ground0 = (Vector3){ -GridSize, 0.0f, -GridSize };
Vector3 Ground1 = (Vector3){ -GridSize, 0.0f,  GridSize };
Vector3 Ground2 = (Vector3){  GridSize, 0.0f,  GridSize };
Vector3 Ground3 = (Vector3){  GridSize, 0.0f, -GridSize };

Ray PickingRay = { 0 };        // Picking ray

RayCollision Collision = { 0 };
Model *CollisionObject = nullptr;

Light Lights[4] = { 0 };
Light InstancingLights[4] = { 0 };

bool ElementErase = true;
bool ElementLines = true;
bool ElementObjects = true;
bool ElementModels = true;
bool ElementUi = true;
bool ElementText = true;

Font FontDefault = { 0 };
Font FontSDF = { 0 };

Image InformationImage;
RenderTexture2D InformationTexture;

float cycle;

Vector2 FontPosition;
Vector2 TextSize = { 0.0f, 0.0f };
float FontSize = 16.0f;
int CurrentFont = 0;            // 0 - fontDefault, 1 - FontSDF

int Dynamic = 0;

// extern const int ScreenWidth;
// extern const int ScreenHeight;

const char Msg[50] = "Signed Distance Fields";

int Layout = 0;
float LayoutFraction = 0;

int FrameRateIndex = 2;
int FrameRateIndexSet = 2;

int FrameRate = 60;
bool AmbientLight = true;

//----------------------------------------------------------------------------------
const int ScreenWidth = 640;
const int ScreenHeight = 480;

void InitGameplayScreen();
void UpdateDrawFrame();
void UnloadGameplayScreen();
void DrawGameplayScreen();
void UpdateGameplayScreen();

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(ScreenWidth, ScreenHeight, "SPOKE REMOTE");

    SetWindowState(FLAG_WINDOW_RESIZABLE);

    // InitAudioDevice();      // Initialize audio device

    // Load global data (assets that must be available in all screens, i.e. font)
    font = LoadFont("resources/mecha.png");
    // music = LoadMusicStream("resources/ambient.ogg");
    // fxCoin = LoadSound("resources/coin.wav");

    // SetMusicVolume(music, 1.0f);
    // PlayMusicStream(music);

    // Setup and init first screen
    // currentScreen = GAMEPLAY;
    InitGameplayScreen();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload current screen data before closing
    UnloadGameplayScreen(); 

    // Unload global data loaded
    UnloadFont(font);
    //  UnloadMusicStream(music);
    //  UnloadSound(fxCoin// );

    // CloseAudioDevice();     // Close audio context

    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}


// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    // TODO: Initialize GAMEPLAY screen variables here!
    framesCounter = 0;
    finishScreen = 0;

    // Define the GameCamera to look into our 3d world
    GameCamera.position = (Vector3){ 0.0f, 20.0f, 60.0f };
    GameCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    GameCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    GameCamera.fovy = 45.0f;
    GameCamera.projection = CAMERA_PERSPECTIVE;

    cycle = 0;

    // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
    // float cameraPos[3] = { GameCamera.position.x, GameCamera.position.y, GameCamera.position.z };
    // SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

    SpokeObject = LoadModelFromMesh(GenMeshCube( 10.0f, 10.0f, 10.0f ));
    SpokeObjectBoundingBox = GetModelBoundingBox( SpokeObject );

    GameCubeMesh = GenMeshCube( 2.0f, 2.0f, 2.0f );
    GameCube = LoadModelFromMesh(GenMeshCube( 2.0f, 2.0f, 2.0f ));
    GameSphere = LoadModelFromMesh( GenMeshSphere( 2.0, 32, 32 ));
    GameTorus = LoadModelFromMesh( GenMeshTorus( 0.5, 2.0, 32, 32 ));
    GameCone = LoadModelFromMesh( GenMeshCone( 0.5f, 2.0f, 32 ) );
    GameCylinder = LoadModelFromMesh( GenMeshCylinder( 1.0f, 2.0f, 32 ) );

    BoundingBox cubeBoundingBox = GetModelBoundingBox( GameCube ); 

    GameShader = LoadShader(TextFormat("resources/shaders/glsl%i/lighting.vs", GLSL_VERSION),
                            TextFormat("resources/shaders/glsl%i/lighting.fs", GLSL_VERSION));
    
    // Get some required shader locations
    GameShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(GameShader, "viewPos");
    // NOTE: "matModel" location name is automatically assigned on shader loading, 
    // no need to get the location again if using that uniform name
    // GameShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(GameShader, "matModel");

    // Ambient light level (some basic lighting)
    ShaderAmbientLoc = GetShaderLocation(GameShader, "ambient");
    float ambient[4] = { 2.0f, 2.0f, 2.0f, 1.0f };
    SetShaderValue(GameShader, ShaderAmbientLoc, ambient, SHADER_UNIFORM_VEC4);

    // Load lighting shader
    InstancingShader = LoadShader(TextFormat("resources/shaders/glsl%i/lighting_instancing.vs", GLSL_VERSION),
                                  TextFormat("resources/shaders/glsl%i/lighting.fs", GLSL_VERSION));
    // Get shader locations
    InstancingShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(InstancingShader, "mvp");
    InstancingShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(InstancingShader, "viewPos");
    InstancingShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(InstancingShader, "instanceTransform");

    // Set shader value: ambient light level
    InstancingAmbientLoc = GetShaderLocation(InstancingShader, "ambient");
    float instancingAmbient[4] = { 2.0f, 2.0f, 2.0f, 2.0f };
    SetShaderValue(InstancingShader, InstancingAmbientLoc, instancingAmbient, SHADER_UNIFORM_VEC4);

    // Create one light
    // CreateLight(LIGHT_DIRECTIONAL, (Vector3){ GridSize, GridSize, 0.0f }, Vector3Zero(), WHITE, InstancingShader);

    // Create Lights
    ClearLightIndex();
    InstancingLights[0] = CreateLight(LIGHT_POINT, (Vector3){ 0, 8, 20 }, Vector3Zero(), WHITE,    InstancingShader);
    InstancingLights[1] = CreateLight(LIGHT_POINT, (Vector3){ 32, 32, 32 }, Vector3Zero(), RED,    InstancingShader);
    InstancingLights[2] = CreateLight(LIGHT_POINT, (Vector3){ -32, 32, 32 }, Vector3Zero(), GREEN, InstancingShader);
    InstancingLights[3] = CreateLight(LIGHT_POINT, (Vector3){ 32, 32, -32 }, Vector3Zero(), BLUE,  InstancingShader);

    InstancingLights[0].enabled = true;
    InstancingLights[1].enabled = false;
    InstancingLights[2].enabled = false;
    InstancingLights[3].enabled = false;

    CubeInstanceCount = pow(2,8);
    CubeInstances = (Matrix *)RL_CALLOC(CubeInstanceCount, sizeof(Matrix));  

    MatInstances = LoadMaterialDefault();
    MatInstances.shader = InstancingShader;
    MatInstances.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    // Create Lights
    ClearLightIndex();
    Lights[0] = CreateLight(LIGHT_POINT, (Vector3){ 0, 8, 20 }, Vector3Zero(), WHITE, GameShader);
    Lights[1] = CreateLight(LIGHT_POINT, (Vector3){ 32, 32, 32 }, Vector3Zero(), RED, GameShader);
    Lights[2] = CreateLight(LIGHT_POINT, (Vector3){ -32, 32, 32 }, Vector3Zero(), GREEN, GameShader);
    Lights[3] = CreateLight(LIGHT_POINT, (Vector3){ 32, 32, -32 }, Vector3Zero(), BLUE, GameShader);

    Lights[0].enabled = true;
    Lights[1].enabled = false;
    Lights[2].enabled = false;
    Lights[3].enabled = false;

    // Assign out lighting shader to model
    GameCube.materials[0].shader = GameShader;
    GameSphere.materials[0].shader = GameShader;
    GameTorus.materials[0].shader = GameShader;
    GameCylinder.materials[0].shader = GameShader;
    GameCone.materials[0].shader = GameShader;

    SpokeObject.materials[0].shader = GameShader;

    // Default font generation from TTF font
    // Loading file to memory
    int fileSize = 0;
    unsigned char *fileData = LoadFileData("resources/anonymous_pro_bold.ttf", &fileSize);
    
    FontDefault.baseSize = 16;
    FontDefault.glyphCount = 95;

    // Loading font data from memory data
    // Parameters > font size: 16, no glyphs array provided (0), glyphs count: 95 (autogenerate chars array)
    FontDefault.glyphs = LoadFontData(fileData, fileSize, 16, 0, 95, FONT_DEFAULT);
    // Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 4 px, pack method: 0 (default)
    Image atlas = GenImageFontAtlas(FontDefault.glyphs, &FontDefault.recs, 95, 16, 4, 0);
    FontDefault.texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);

    // SDF font generation from TTF font (same font)
    FontSDF.baseSize = 16;
    FontSDF.glyphCount = 95;
    // Parameters > font size: 16, no glyphs array provided (0), glyphs count: 0 (defaults to 95)
    FontSDF.glyphs = LoadFontData(fileData, fileSize, 16, 0, 0, FONT_SDF);
    // Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 0 px, pack method: 1 (Skyline algorythm)
    atlas = GenImageFontAtlas(FontSDF.glyphs, &FontSDF.recs, 95, 16, 0, 1);
    FontSDF.texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);

    UnloadFileData(fileData);      // Free memory from loaded file

    // Load SDF required shader (we use default vertex shader)
    FontShader = LoadShader(0, TextFormat("resources/shaders/glsl%i/sdf.fs", GLSL_VERSION));
    SetTextureFilter(FontSDF.texture, TEXTURE_FILTER_BILINEAR);    // Required for SDF font

    InformationImage = GenImageColor( 100, 60, WHITE );
    InformationTexture = LoadRenderTexture(200,60);
    SetTextureFilter(InformationTexture.texture, TEXTURE_FILTER_BILINEAR);

    ImageClearBackground( &InformationImage, WHITE );
    ImageDrawLine( &InformationImage, 0,0,100,30,BLACK);
    // InformationTexture = LoadTextureFromImage( InformationImage );

    BeginTextureMode(InformationTexture);
        BeginDrawing();
            BeginShaderMode( FontShader);    // Activate SDF font shader
                DrawTextEx(FontSDF, "SPHERE", (Vector2){-1,0}, 64, 0, DARKGRAY);
            EndShaderMode();            // Activate our default shader for next drawings
        EndDrawing();
    EndTextureMode();

/*
    GameModel = LoadModel( "resources/models/robot.glb");   // Load new model
    // GameModel = LoadModel( "resources/models/cesium_man.m3d");   // Load new model
    // model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture; // Set current map diffuse texture
    for ( int i = 0; i < GameModel.materialCount; i++ )
        GameModel.materials[i].shader = GameShader;

    GameModelBounds = GetMeshBoundingBox(GameModel.meshes[0]);

    GameEsp32 = LoadModel( "resources/models/cb_esp32.glb" );
    for ( int i = 0; i < GameEsp32.materialCount; i++ )
        GameEsp32.materials[i].shader = GameShader;

    GameStlMesh = load_stl( "resources/models/StudyMinimalSkeleton.stl" );
    GameStl = LoadModelFromMesh(GameStlMesh);
    GameStl.materials[0].shader = GameShader;
    // GameStl.materials[0].maps[0].color = ORANGE;
*/
    // Load default style
    GuiLoadStyleDefault();

    GuiSetFont( FontSDF );

    GuiSetStyle(DEFAULT, TEXT_SIZE, 24 );

    GuiSetStyle(DEFAULT, BACKGROUND_COLOR,     0xFFFFFFB0);
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL,  0x404040BF);
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,    0xFFFFFFBF);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,    0x404040BF);
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, 0xFF9090BF);
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED,   0xFFFFFFBF);
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,   0xFF4040BF);
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, 0xFF9090BF);
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED,   0xFFAAAABF);
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,   0xFF4040BF);
    GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, 0xb5c1c2ff);
    GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, 0xe6e9e9ff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, 0xaeb7b8ff);
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 2);
    GuiSetStyle(DEFAULT, TEXT_PADDING, 0);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    
    GuiSetStyle(DEFAULT, LINE_COLOR, 0x4040407F );

    GuiSetStyle(COMBOBOX, COMBO_BUTTON_WIDTH, 50 );

    GuiSetStyle(TOGGLE, BORDER_COLOR_PRESSED, 0x909090BF);
    GuiSetStyle(TOGGLE, BASE_COLOR_PRESSED,   0xAAAAAABF);
    GuiSetStyle(TOGGLE, TEXT_COLOR_PRESSED,   0x404040BF);
    GuiSetStyle(TOGGLE, BORDER_COLOR_FOCUSED, 0xFF9090BF);
    GuiSetStyle(TOGGLE, BASE_COLOR_FOCUSED,   0xFFAAAABF);
    GuiSetStyle(TOGGLE, TEXT_COLOR_FOCUSED,   0xFF4040BF);

    GuiSetStyle(SLIDER, BORDER_COLOR_PRESSED, 0x909090BF);
    GuiSetStyle(SLIDER, BASE_COLOR_PRESSED,   0xAAAAAABF);
    GuiSetStyle(SLIDER, TEXT_COLOR_PRESSED,   0x404040BF);
    GuiSetStyle(SLIDER, BORDER_COLOR_FOCUSED, 0xFF9090BF);
    GuiSetStyle(SLIDER, BASE_COLOR_FOCUSED,   0xFFAAAABF);
    GuiSetStyle(SLIDER, TEXT_COLOR_FOCUSED,   0xFF4040BF);

    GuiSetStyle( BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER );

}

// Update and draw game frame
void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    // UpdateMusicStream(music);       // NOTE: Music keeps playing between screens

    UpdateGameplayScreen();


    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawGameplayScreen();
        
    EndDrawing();
    //----------------------------------------------------------------------------------
}


// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    // TODO: Update GAMEPLAY screen variables here!

    if ( IsGestureDetected(GESTURE_TAP) ) {

    }

    if (IsKeyDown(KEY_LEFT) ) {
        GameCamera.position = Vector3RotateByAxisAngle( GameCamera.position, GameCamera.up, -0.01 );
    }

    if ( IsKeyDown( KEY_RIGHT ) ){
        GameCamera.position = Vector3RotateByAxisAngle( GameCamera.position, GameCamera.up, 0.01     );
    }

    if (IsKeyDown(KEY_UP) ) {
        Vector3 axis = Vector3CrossProduct( GameCamera.position, GameCamera.up );
        GameCamera.position = Vector3RotateByAxisAngle( GameCamera.position, axis, -0.01 );
    }

    if ( IsKeyDown( KEY_DOWN ) ){
        Vector3 axis = Vector3CrossProduct( GameCamera.position, GameCamera.up );
        GameCamera.position = Vector3RotateByAxisAngle( GameCamera.position, axis, 0.01     );
    }

    if (IsKeyPressed(KEY_W)) { 
        Lights[0].enabled = !Lights[0].enabled; 
        InstancingLights[0].enabled = !InstancingLights[0].enabled; 
    }
    if (IsKeyPressed(KEY_R)) { 
        Lights[1].enabled = !Lights[1].enabled; 
        InstancingLights[1].enabled = !InstancingLights[1].enabled; 
    }
    if (IsKeyPressed(KEY_G)) { 
        Lights[2].enabled = !Lights[2].enabled; 
        InstancingLights[2].enabled = !InstancingLights[2].enabled; 
    }
    if (IsKeyPressed(KEY_B)) { 
        Lights[3].enabled = !Lights[3].enabled; 
        InstancingLights[3].enabled = !InstancingLights[3].enabled; 
    }
    if (IsKeyPressed(KEY_E)) { 
        ElementErase = !ElementErase; 
    }
    if (IsKeyPressed(KEY_L)) { 
        ElementLines = !ElementLines; 
    }
    if (IsKeyPressed(KEY_M)) { 
        ElementModels = !ElementModels; 
    }
    if (IsKeyPressed(KEY_U)) { 
        ElementUi = !ElementUi; 
    }
    if (IsKeyPressed(KEY_T)) { 
        ElementText = !ElementText; 
    }

    // Update light values (actually, only enable/disable them)
    for (int i = 0; i < 4; i++) {
        UpdateLightValues(GameShader, Lights[i]);
        UpdateLightValues(InstancingShader, InstancingLights[i] );
    }

    // Press enter or tap to change to ENDING screen
    if (IsKeyPressed(KEY_ENTER) )
    {
        finishScreen = 1;
        // PlaySound(fxCoin);
    }

    FontSize += GetMouseWheelMove()*8.0f;

    if (FontSize < 6) 
        FontSize = 6;

    if (IsKeyDown(KEY_SPACE)) 
        CurrentFont = 1;
    else 
        CurrentFont = 0;

    if ( FrameRateIndex != FrameRateIndexSet ) {
        int fps = 30;
        switch ( FrameRateIndex ) {
            case 0:
                fps = 10;
                break;
            case 1:
                fps = 30;
                break;
            case 2:
                fps = 60;
                break;
            case 3:
                fps = 120;
                break;
            case 4:
                fps = 160;
                break;
            case 5:
                fps = 220;
                break;
        }
        SetTargetFPS( fps );
        FrameRateIndexSet = FrameRateIndex;
    }

    // Ambient light level (some basic lighting)
    float f[4];
    if ( AmbientLight ) {
        f[0] = 1.0f;
        f[1] = 1.0f;
        f[2] = 1.0f;
        f[3] = 1.0f;
    } else {
        f[0] = 0.0f;
        f[1] = 0.0f;
        f[2] = 0.0f;
        f[3] = 0.0f;
    } 
    SetShaderValue(GameShader, ShaderAmbientLoc, f, SHADER_UNIFORM_VEC4);
    SetShaderValue(InstancingShader, InstancingAmbientLoc, f, SHADER_UNIFORM_VEC4);

    // Get ray and test against objects
    PickingRay = GetScreenToWorldRay(GetMousePosition(), GameCamera);
    // printf( "    %f %f %f\n", PickingRay.direction.x, PickingRay.direction.y, PickingRay.direction.z );

    CollisionObject = nullptr;
    Collision.distance = FLT_MAX;
    Collision.hit = false;

    // Check ray collision against ground quad
    RayCollision groundHitInfo = GetRayCollisionQuad(PickingRay, Ground0, Ground1, Ground2, Ground3 );

    if ((groundHitInfo.hit) && (groundHitInfo.distance < Collision.distance)) {
        Collision = groundHitInfo;

        printf( "HIT GROUND %f\n", groundHitInfo.distance );
    }

    // Check ray collision against test sphere
    RayCollision spokeObjectHitInfo = GetRayCollisionBox( PickingRay, SpokeObjectBoundingBox );

    if ((spokeObjectHitInfo.hit) && (spokeObjectHitInfo.distance < Collision.distance)) {
        Collision = spokeObjectHitInfo;
        CollisionObject = &SpokeObject;
        printf( "HIT SPOKE OBJECT %f\n", spokeObjectHitInfo.distance );
    }


    // // Check ray collision against test sphere
    // RayCollision cubeHitInfo = GetRayCollisionSphere(PickingRay, (Vector3){ 0, 0, 0 }, 10.0F);

    // if ((cubeHitInfo.hit) && (cubeHitInfo.distance < Collision.distance)) {
    //     Collision = cubeHitInfo;
    //     CollisionObject = &SpokeObject;
    //     printf( "HIT CUBE %f\n", cubeHitInfo.distance );
    // }

}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    if ( ElementErase ) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    }
    
    float cameraPos[3] = { GameCamera.position.x, GameCamera.position.y, GameCamera.position.z };
    SetShaderValue(GameShader, GameShader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(InstancingShader, InstancingShader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

    Vector2 pos = { 20, 10 };
                
    if ( ElementText) {
        BeginShaderMode( FontShader);    // Activate SDF font shader
            DrawTextEx(FontSDF, "SPOKE REMOTE", pos, font.baseSize*3.0f, 4, MAROON);
        EndShaderMode();
    }

    BeginMode3D(GameCamera);
        if ( ElementLines ) {
            DrawGrid( GridSlices, GridSpacing );        // Draw a grid
        }

        Vector3 p = (Vector3){ 0, 0, 0 };

        DrawModel( SpokeObject, p, 1.0f, ( CollisionObject == &SpokeObject ) ? RED : LIGHTGRAY );

    EndMode3D();

/*

        // gradually change layout fraction (for animations)
        if ( (float)Layout != LayoutFraction ) {
            if ( Layout > LayoutFraction ) {
                LayoutFraction += 0.01;
                if ( LayoutFraction > Layout )
                    LayoutFraction = Layout;
            } else {
                LayoutFraction -= 0.01;
                if ( LayoutFraction < Layout )
                    LayoutFraction = Layout;
            } 
        }


        if ( ElementModels ) {
            for ( int i = 0; i <= 35; i++ ) {
                Vector3 a = (Vector3){ (i-13.5f)*4.0f, 0, 0 };
                Vector3 b = (Vector3){ ((i)/6-2.5f)*4.0f, 0, ((i)%6-2.5f)*4.0f };

                Vector3 p = Vector3Lerp( a, b, LayoutFraction );
                DrawModel( GameCube, p, 1.0f, LIGHTGRAY );
            }
        }

        if ( Dynamic ) {
            cycle += 0.01;
        }

        // // Old busted joint
        // int n = pow(2,16);
        // float nsqr = sqrt( n );
        // for ( int i = 0; i < n; i++ ) {
        //     Vector3 l = (Vector3){ ((i)/nsqr-nsqr/2-0.5)*1.0f, -10, ((i)%(int)nsqr-nsqr/2-0.5)*1.0f };    
        //     DrawModel( GameCube, l, 0.25f, WHITE );
        // }

        #if ( 0 )

            float nisqr = sqrt( CubeInstanceCount );

            float scale = 0.25F;
            Matrix matScale = MatrixScale(scale, scale, scale);
            Matrix matRotation = MatrixRotate((Vector3){0,1,0}, 0 );
            for ( int i = 0; i < CubeInstanceCount; i++ ) {
                Vector3 l = (Vector3){ ((i)/(int)nisqr-nisqr/2-0.5f)*1.0f, -12, ((i)%(int)nisqr-nisqr/2-0.5f)*1.0f };    
                Matrix matTranslation = MatrixTranslate(l.x, l.y, l.z);

                CubeInstances[i] = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
            }

            DrawMeshInstanced( GameCubeMesh, MatInstances, CubeInstances, CubeInstanceCount );

        #endif 

        if ( ElementText ) {
            BeginShaderMode( FontShader);    // Activate SDF font shader
                Vector3 mt = MeasureText3D(FontSDF, "SPHERE", 32,0, 0 );

                DrawText3D( FontSDF, "SPHERE", (Vector3){ 22.0f*sin(cycle)-mt.x/2, -5.0f, 22.0f*cos(cycle)}, 32, 5, 0.0, true, GRAY );
            EndShaderMode();
        }

        Vector3 spherePosition = (Vector3){ 22.0f*sin(cycle), 0.0f, 22.0f*cos(cycle) };

        if ( ElementModels ) {
            DrawModel( GameSphere, spherePosition, 1.0f, LIGHTGRAY );
        }

        if ( ElementText ) {
            Vector2 size = { 1.0f, 1.0f };
            Rectangle source = { 0.0f, 0.0f, (float)InformationTexture.texture.width, -(float)InformationTexture.texture.height };
            DrawBillboardRec( GameCamera, InformationTexture.texture, source, (Vector3){ 22.0f*sin(cycle), 4.0f, 22.0f*cos(cycle)}, size, WHITE );
        }

        Vector3 points[4];

        for ( int i = -20; i < 20; i++ ) {
            if ( ElementModels || ElementLines ) {
                points[ 0 ] = (Vector3){ 2.0f * i, 4.0f*sin(3*cycle)+4.0f, -16.0f * sin(cycle)};
                points[ 3 ] = (Vector3){ 2.0f * i, -4.0f*sin(4*cycle)+4.0f, 16.0f * sin( cycle)};

                points[ 1 ] = points[ 0 ];
                points[ 1 ].y -= 15;
                points[ 2 ] = points[ 3 ];
                points[ 2 ].y += 10;
            }

            if ( ElementModels ) {
                DrawModel( GameSphere, points[ 0 ], 0.2f, LIGHTGRAY );
                DrawModel( GameCube, points[ 3 ], 0.2f, LIGHTGRAY );
            }

            // DrawSplineSegmentBezierCubic3D( points[0], points[1], points[2], points[3], 24, LIGHTGRAY, false );

            if ( ElementLines ) {
                points[ 2 ] = spherePosition;
                points[ 2 ].y -= 15;
                points[ 3 ] = spherePosition;
                DrawSplineSegmentBezierCubic3D( points[0], points[1], points[2], points[3], 24, LIGHTGRAY, false );
            }
        }

        // if ( ElementModels ) {

        //     Vector3 modelPosition = (Vector3){ 20.0f*sin(cycle), 0.0f, -20.0f*cos(cycle) };
        //     DrawModel( GameModel, modelPosition, 1.5f, WHITE);        // Draw 3d model with texture

        //     modelPosition.y += 8;
        //     DrawModel( GameEsp32, modelPosition, 0.1f, WHITE);        // Draw 3d model with texture

        //     modelPosition.y += 10;
        //     DrawModel( GameStl, modelPosition, 0.1f, RED);        // Draw 3d model with texture
        // }

        if ( ElementLines ) {
            DrawGrid( 20, 10.0f );        // Draw a grid
        }

    EndMode3D();

*/    

    if ( ElementUi ) {
        BeginShaderMode( FontShader);    // Activate SDF font shader

            // GuiDrawRectangle( (Rectangle){ 5, 115, 410, 290 }, 1, WHITE, WHITE );
            GuiPanel( (Rectangle){ 20, 70, 340, 410 }, 0 );
            // GuiGroupBox( (Rectangle){ 10, 120, 400, 200 }, "Viz Control" );

            GuiSetStyle( DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT );
            GuiLabel((Rectangle){ 35, 80, 90, 32 }, "Layout");
            GuiLabel((Rectangle){ 35, 120, 90, 32 }, "Run");
            GuiLabel((Rectangle){ 35, 160, 90, 32 }, "FPS" );
            GuiLabel((Rectangle){ 35, 240, 90, 32 }, "Lights" );
            GuiLabel((Rectangle){ 35, 280, 90, 32 }, "Element" );
            GuiSetStyle( DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER );

            GuiComboBox((Rectangle){ 130, 80, 200, 32 }, "LayoutA;LayoutB", &Layout);

            GuiSetIconScale(1);

            GuiToggleSlider( (Rectangle){ 130, 120, 200, 32 }, "Static;Dynamic", &Dynamic );

            int fps = GetFPS();
            GuiValueBox((Rectangle){ 130, 160, 200, 32 }, 0, &fps, 0, 1000, false );

            GuiComboBox((Rectangle){ 130, 200, 200, 32 }, "10;30;60;120;160;220", &FrameRateIndex );

            GuiToggle( (Rectangle){ 130, 240, 30, 32 }, "W", &Lights[0].enabled );
            GuiToggle( (Rectangle){ 170, 240, 30, 32 }, "R", &Lights[1].enabled );
            GuiToggle( (Rectangle){ 210, 240, 30, 32 }, "G", &Lights[2].enabled );
            GuiToggle( (Rectangle){ 250, 240, 30, 32 }, "B", &Lights[3].enabled );
            GuiToggle( (Rectangle){ 290, 240, 30, 32 }, "A", &AmbientLight );
            for ( int i = 0; i < 4; i++ )
                InstancingLights[i].enabled=Lights[i].enabled;

            GuiToggle( (Rectangle){ 130, 280, 200, 32 }, "Lines", &ElementLines );
            GuiToggle( (Rectangle){ 130, 320, 200, 32 }, "Models", &ElementModels );
            GuiToggle( (Rectangle){ 130, 360, 200, 32 }, "Text", &ElementText );
            GuiToggle( (Rectangle){ 130, 400, 200, 32 }, "UI", &ElementUi );
            GuiToggle( (Rectangle){ 130, 440, 200, 32 }, "Erase", &ElementErase );

        EndShaderMode();
    } else {
        BeginShaderMode( FontShader);    // Activate SDF font shader

            // GuiPanel( (Rectangle){ 20, 70, 60, 52 }, 0 );

            GuiSetStyle( DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER );

            // GuiGroupBox( (Rectangle){ 10, 120, 400, 200 }, "Viz Control" );
            GuiToggle( (Rectangle){ 20, 70, 40, 32 }, "UI", &ElementUi );

        EndShaderMode();

    }
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    // TODO: Unload GAMEPLAY screen variables here!
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}

// Draw codepoint at specified position in 3D space
static void DrawTextCodepoint3D(Font font, int codepoint, Vector3 position, float fontSize, bool backface, Color tint)
{
    // Character index position in sprite font
    // NOTE: In case a codepoint is not available in the font, index returned points to '?'
    int index = GetGlyphIndex(font, codepoint);
    float scale = fontSize/(float)font.baseSize;

    // Character destination rectangle on screen
    // NOTE: We consider charsPadding on drawing
    position.x += (float)(font.glyphs[index].offsetX - font.glyphPadding)/(float)font.baseSize*scale;
    position.z += (float)(font.glyphs[index].offsetY - font.glyphPadding)/(float)font.baseSize*scale;

    // Character source rectangle from font texture atlas
    // NOTE: We consider chars padding when drawing, it could be required for outline/glow shader effects
    Rectangle srcRec = { font.recs[index].x - (float)font.glyphPadding, font.recs[index].y - (float)font.glyphPadding,
                         font.recs[index].width + 2.0f*font.glyphPadding, font.recs[index].height + 2.0f*font.glyphPadding };

    float width = (float)(font.recs[index].width + 1.0f*font.glyphPadding)/(float)font.baseSize*scale;
    float height = (float)(font.recs[index].height + 1.0f*font.glyphPadding)/(float)font.baseSize*scale;

    if (font.texture.id > 0)
    {
        const float x = 0.0f;
        const float y = 0.0f;
        const float z = 0.0f;

        // normalized texture coordinates of the glyph inside the font texture (0.0f -> 1.0f)
        const float tx = srcRec.x/font.texture.width;
        const float ty = srcRec.y/font.texture.height;
        const float tw = (srcRec.x+srcRec.width)/font.texture.width;
        const float th = (srcRec.y+srcRec.height)/font.texture.height;

        if (SHOW_LETTER_BOUNDRY) DrawCubeWiresV((Vector3){ position.x + width/2, position.y, position.z + height/2}, (Vector3){ width, LETTER_BOUNDRY_SIZE, height }, LETTER_BOUNDRY_COLOR);

        rlCheckRenderBatchLimit(4 + 4*backface);
        rlSetTexture(font.texture.id);

        rlPushMatrix();
            rlTranslatef(position.x, position.y, position.z);
            rlRotatef(90,1.0f,0.0f,0.0f);

            rlBegin(RL_QUADS);
                rlColor4ub(tint.r, tint.g, tint.b, tint.a);

                // Front Face
                rlNormal3f(0.0f, 1.0f, 0.0f);                                   // Normal Pointing Up
                rlTexCoord2f(tx, ty); rlVertex3f(x,         y, z);              // Top Left Of The Texture and Quad
                rlTexCoord2f(tx, th); rlVertex3f(x,         y, z + height);     // Bottom Left Of The Texture and Quad
                rlTexCoord2f(tw, th); rlVertex3f(x + width, y, z + height);     // Bottom Right Of The Texture and Quad
                rlTexCoord2f(tw, ty); rlVertex3f(x + width, y, z);              // Top Right Of The Texture and Quad

                if (backface)
                {
                    // Back Face
                    rlNormal3f(0.0f, -1.0f, 0.0f);                              // Normal Pointing Down
                    rlTexCoord2f(tx, ty); rlVertex3f(x,         y, z);          // Top Right Of The Texture and Quad
                    rlTexCoord2f(tw, ty); rlVertex3f(x + width, y, z);          // Top Left Of The Texture and Quad
                    rlTexCoord2f(tw, th); rlVertex3f(x + width, y, z + height); // Bottom Left Of The Texture and Quad
                    rlTexCoord2f(tx, th); rlVertex3f(x,         y, z + height); // Bottom Right Of The Texture and Quad
                }
            rlEnd();
        rlPopMatrix();

        rlSetTexture(0);
    }
}

// Draw a 2D text in 3D space
static void DrawText3D(Font font, const char *text, Vector3 position, float fontSize, float fontSpacing, float lineSpacing, bool backface, Color tint)
{
    int length = TextLength(text);          // Total length in bytes of the text, scanned by codepoints in loop

    float textOffsetY = 0.0f;               // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;               // Offset X to next character to draw

    float scale = fontSize/(float)font.baseSize;

    for (int i = 0; i < length;)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;

        if (codepoint == '\n')
        {
            // NOTE: Fixed line spacing of 1.5 line-height
            // TODO: Support custom line spacing defined by user
            textOffsetY += scale + lineSpacing/(float)font.baseSize*scale;
            textOffsetX = 0.0f;
        }
        else
        {
            if ((codepoint != ' ') && (codepoint != '\t')) {
                DrawTextCodepoint3D(font, codepoint, (Vector3){ position.x + textOffsetX, position.y, position.z + textOffsetY }, fontSize, backface, tint);
            }

            if (font.glyphs[index].advanceX == 0) 
                textOffsetX += (float)(font.recs[index].width + fontSpacing)/(float)font.baseSize*scale;
            else 
                textOffsetX += (float)(font.glyphs[index].advanceX + fontSpacing)/(float)font.baseSize*scale;
        }

        i += codepointByteCount;   // Move text bytes counter to next codepoint
    }
}

// Measure a text in 3D. For some reason `MeasureTextEx()` just doesn't seem to work so i had to use this instead.
static Vector3 MeasureText3D(Font font, const char* text, float fontSize, float fontSpacing, float lineSpacing)
{
    int len = TextLength(text);
    int tempLen = 0;                // Used to count longer text line num chars
    int lenCounter = 0;

    float tempTextWidth = 0.0f;     // Used to count longer text line width

    float scale = fontSize/(float)font.baseSize;
    float textHeight = scale;
    float textWidth = 0.0f;

    int letter = 0;                 // Current character
    int index = 0;                  // Index position in sprite font

    for (int i = 0; i < len; i++)
    {
        lenCounter++;

        int next = 0;
        letter = GetCodepoint(&text[i], &next);
        index = GetGlyphIndex(font, letter);

        // NOTE: normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol so to not skip any we set next = 1
        if (letter == 0x3f) next = 1;
        i += next - 1;

        if (letter != '\n')
        {
            if (font.glyphs[index].advanceX != 0) textWidth += (font.glyphs[index].advanceX+fontSpacing)/(float)font.baseSize*scale;
            else textWidth += (font.recs[index].width + font.glyphs[index].offsetX)/(float)font.baseSize*scale;
        }
        else
        {
            if (tempTextWidth < textWidth) tempTextWidth = textWidth;
            lenCounter = 0;
            textWidth = 0.0f;
            textHeight += scale + lineSpacing/(float)font.baseSize*scale;
        }

        if (tempLen < lenCounter) tempLen = lenCounter;
    }

    if (tempTextWidth < textWidth) tempTextWidth = textWidth;

    Vector3 vec = { 0 };
    vec.x = tempTextWidth + (float)((tempLen - 1)*fontSpacing/(float)font.baseSize*scale); // Adds chars spacing to measure
    vec.y = 0.25f;
    vec.z = textHeight;

    return vec;
}

// // Draw a line in 3D world space
// void DrawSpline3D(Vector3 startPos, Vector3 endPos, Color color)
// {
//     rlBegin(RL_LINES);
//         rlColor4ub(color.r, color.g, color.b, color.a);
//         rlVertex3f(startPos.x, startPos.y, startPos.z);
//         rlVertex3f(endPos.x, endPos.y, endPos.z);
//     rlEnd();
// }


// Draw spline segment: Cubic Bezier, 2 points, 2 control points
void DrawSplineSegmentBezierCubic3D(Vector3 p1, Vector3 c2, Vector3 c3, Vector3 p4, int segments, Color color, bool debugIt )
{
    const float step = 1.0f/segments;

    Vector3 previous = p1;
    Vector3 current = { 0 };
    float t = 0.0f;

    for (int i = 1; i <= segments; i++)
    {
        t = step*(float)i;

        float a = powf(1.0f - t, 3);
        float b = 3.0f*powf(1.0f - t, 2)*t;
        float c = 3.0f*(1.0f - t)*powf(t, 2);
        float d = powf(t, 3);

        current.x = a*p1.x + b*c2.x + c*c3.x + d*p4.x;
        current.y = a*p1.y + b*c2.y + c*c3.y + d*p4.y;
        current.z = a*p1.z + b*c2.z + c*c3.z + d*p4.z;

        // float dx = current.x - previous.x;
        // float dy = current.y - previous.y;
        // float dz = current.z - previous.z;
        // float size = 0.5f*thick/sqrtf(dx*dx+dy*dy+dz*dz);

        if ( debugIt )
            printf( " %d   [%f,%f,%f] - [%f,%f,%f] \n", i, previous.x, previous.y, previous.z, current.x, current.y, current.z );

        DrawLine3D( previous, current, color );

        previous = current;
    }
}

// Draw spline: B-Spline, minimum 4 points
void DrawSplineBasis3D(Vector3 *points, int pointCount, Color color)
{
    if (pointCount < 4) return;

    float a[4] = { 0 };
    float b[4] = { 0 };
    float c[4] = { 0 };
    // float dy = 0.0f;
    // float dx = 0.0f;
    // float dz = 0.0f;
    // float size = 0.0f;

    Vector3 currentPoint = { 0 };
    Vector3 nextPoint = { 0 };
    // Vector3 vertices[2*SPLINE_SEGMENT_DIVISIONS + 2] = { 0 };

    for (int i = 0; i < (pointCount - 3); i++)
    {
        float t = 0.0f;

        Vector3 p1 = points[i], p2 = points[i + 1], p3 = points[i + 2], p4 = points[i + 3];

        a[0] = (-p1.x + 3.0f*p2.x - 3.0f*p3.x + p4.x)/6.0f;
        a[1] = (3.0f*p1.x - 6.0f*p2.x + 3.0f*p3.x)/6.0f;
        a[2] = (-3.0f*p1.x + 3.0f*p3.x)/6.0f;
        a[3] = (p1.x + 4.0f*p2.x + p3.x)/6.0f;

        b[0] = (-p1.y + 3.0f*p2.y - 3.0f*p3.y + p4.y)/6.0f;
        b[1] = (3.0f*p1.y - 6.0f*p2.y + 3.0f*p3.y)/6.0f;
        b[2] = (-3.0f*p1.y + 3.0f*p3.y)/6.0f;
        b[3] = (p1.y + 4.0f*p2.y + p3.y)/6.0f;

        c[0] = (-p1.z + 3.0f*p2.z - 3.0f*p3.z + p4.z)/6.0f;
        c[1] = (3.0f*p1.z - 6.0f*p2.z + 3.0f*p3.z)/6.0f;
        c[2] = (-3.0f*p1.z + 3.0f*p3.z)/6.0f;
        c[3] = (p1.z + 4.0f*p2.z + p3.z)/6.0f;

        nextPoint.x = a[3];
        nextPoint.y = b[3];
        nextPoint.z = c[3];

        if (i > 0){
            DrawLine3D( currentPoint, nextPoint, color );
        }

        currentPoint = nextPoint;
    }
}
