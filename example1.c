#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "raylib.h"
#include "raymath.h"
#include "raygui.h"
#include "rlgl.h"

void DrawModelMatrix(Model ModelResource, Matrix matTransform, Color tint);
static void DrawSphereWiresMTX(Matrix m, float scale, Color color);

static char *strtok_r(char *s, const char *delim, char **save_ptr)
{
    char *end;
    if (s == NULL)
        s = *save_ptr;
    if (*s == '\0')
    {
        *save_ptr = s;
        return NULL;
    }
    /* Scan leading delimiters.  */
    s += strspn (s, delim);
    if (*s == '\0')
    {
        *save_ptr = s;
        return NULL;
    }
    /* Find the end of the token.  */
    end = s + strcspn (s, delim);
    if (*end == '\0')
    {
        *save_ptr = end;
        return s;
    }
    /* Terminate the token and make *SAVE_PTR point past it.  */
    *end = '\0';
    *save_ptr = end + 1;
    return s;
}

typedef struct 
{
    int     ReferenceCount;
    char    *Name;

    Model   ModelResource;
} Resource_t;

typedef struct Node_t
{
    char            *Name;

    int             ResourceIndex;

    Matrix          localMatrix;
    Matrix          worldMatrix;

    struct Node_t   *Parent;

} Node_t;

typedef struct 
{
    int         nResources;
    int         nNodes;
    Resource_t  *Resources;
    Node_t      *Nodes;
} Scene_T;

//  search backwards from the current Node. 
//  parent is usually not far up the chain 
Node_t *FindNode(Scene_T *scene, Node_t *cNode, const char *parent)
{
    while(cNode!=&scene->Nodes[0])
    {
        cNode--;
        if (cNode->Name!=NULL)
        {
            if (stricmp(cNode->Name,parent)==0)
            {
                return cNode;
            }
        }
    }
    return NULL;
}

void UnLoadAscii3D(Scene_T *scene)
{
    for (int q=0;q<scene->nNodes;q++)
    {
        Node_t *node = &scene->Nodes[q];
        RL_FREE(node->Name);
    }
    for (int q=0;q<scene->nResources;q++)
    {
        UnloadModel(scene->Resources[q].ModelResource);
        RL_FREE(scene->Resources[q].Name);
    }
    RL_FREE(scene->Nodes);
    RL_FREE(scene->Resources);
    RL_FREE(scene);
}

Scene_T *LoadAscii3D(const char *fname)
{
char *linerest = NULL;

    int size = 0;
    char *buffer = LoadFileData(fname,&size);
    if (buffer==NULL)
    {
        TraceLog(LOG_ERROR,"File %s not found",fname);
        return(NULL);
    }
    Scene_T *scene = RL_CALLOC(1,sizeof(Scene_T));

    int resourcesToLoad=0;
    int nodesToParse=0;
    Resource_t *cResource = NULL;
    Node_t *cNode = NULL;
    
    char *line = strtok_r(buffer,"\r\n",&linerest);
    while(line!=NULL)
    {
        char *tokrest = NULL;
        
        if (strstr(line,"resources:")!=NULL)
        {
            char *tok = strtok_r(line,",:",&tokrest);
            scene->nResources = atol(strtok_r(NULL,":",&tokrest))-1;
            scene->Resources = RL_CALLOC(scene->nResources,sizeof(Resource_t));
            resourcesToLoad = scene->nResources;
            cResource = &scene->Resources[0];
        }
        else if (strstr(line,"nodes:")!=NULL)
        {
            char *tok = strtok_r(line,",:",&tokrest);
            scene->nNodes = atol(strtok_r(NULL,",:",&tokrest))-1;
            scene->Nodes = RL_CALLOC(scene->nNodes,sizeof(Node_t));
            cNode = scene->Nodes;
            nodesToParse = scene->nNodes;
        }
        else if (resourcesToLoad!=0)
        {
            cResource->Name = strdup(line);
            cResource->ReferenceCount = 0;
            cResource->ModelResource = LoadModel(cResource->Name);
            cResource++;
            resourcesToLoad--;
        }
        else if (nodesToParse!=0)
        {
            char *Class = strtok_r(line,",:",&tokrest);
            char *Name = strtok_r(NULL,",:",&tokrest);
            char *Parent = strtok_r(NULL,",:",&tokrest);
            int ResourceIndex = atol(strtok_r(NULL,",:",&tokrest));

            if (ResourceIndex!=-1)
            {
                scene->Resources[ResourceIndex].ReferenceCount++;
            }
            cNode->localMatrix = MatrixIdentity();

            float loadedMatrix[12];
            for (int q=0;q<12;q++)
            {
                char *nxt = strtok_r(NULL,",:",&tokrest);
                float val = strtod(nxt,NULL);
                loadedMatrix[q] = val;
            }
            cNode->localMatrix.m0 = loadedMatrix[0];
            cNode->localMatrix.m1 = loadedMatrix[1];
            cNode->localMatrix.m2 = loadedMatrix[2];

            cNode->localMatrix.m4 = loadedMatrix[3];
            cNode->localMatrix.m5 = loadedMatrix[4];
            cNode->localMatrix.m6 = loadedMatrix[5];

            cNode->localMatrix.m8 =  loadedMatrix[6];
            cNode->localMatrix.m9 =  loadedMatrix[7];
            cNode->localMatrix.m10 = loadedMatrix[8];

            cNode->localMatrix.m12 = loadedMatrix[9];
            cNode->localMatrix.m13 = loadedMatrix[10];
            cNode->localMatrix.m14 = loadedMatrix[11];
            cNode->worldMatrix = cNode->localMatrix;

            if (stricmp(Class,"OmniLight3D")==0)
            {
                char *nxt = strtok_r(NULL,",:",&tokrest);
         //       printf("Color %s\n",nxt);
            }
            cNode->ResourceIndex = ResourceIndex;
            cNode->Name = strdup(Name);
            cNode->Parent = FindNode(scene,cNode,Parent);
            cNode++;
            nodesToParse--;
        }
        line = strtok_r(NULL,"\r\n",&linerest);
    }
    return(scene);
}

//  we need to draw a mesh with a direct matrix 
//  instead of trashing the ModelResource matrix every drawcall
void DrawModelMatrix(Model ModelResource, Matrix matTransform, Color tint)
{
    for (int i = 0; i < ModelResource.meshCount; i++)
    {
        Color color = ModelResource.materials[ModelResource.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;
        Color colorTint = WHITE;
        colorTint.r = (unsigned char)((((float)color.r/255.0f)*((float)tint.r/255.0f))*255.0f);
        colorTint.g = (unsigned char)((((float)color.g/255.0f)*((float)tint.g/255.0f))*255.0f);
        colorTint.b = (unsigned char)((((float)color.b/255.0f)*((float)tint.b/255.0f))*255.0f);
        colorTint.a = (unsigned char)((((float)color.a/255.0f)*((float)tint.a/255.0f))*255.0f);
        ModelResource.materials[ModelResource.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = colorTint;
        DrawMesh(ModelResource.meshes[i], ModelResource.materials[ModelResource.meshMaterial[i]], matTransform);
        ModelResource.materials[ModelResource.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = color;
    }
}

void DrawAscii3D(Scene_T *scene)
{
    for (int q=0;q<scene->nNodes;q++)
    {
        Node_t *node = &scene->Nodes[q];
        Matrix m = node->localMatrix;
        if (node->Parent!=NULL)
        {
            m = MatrixMultiply(m,node->Parent->worldMatrix);
        }
        node->worldMatrix = m;
        if (node->ResourceIndex!=-1)
        {
            DrawModelMatrix(scene->Resources[scene->Nodes[q].ResourceIndex].ModelResource,m,WHITE);
        }
    }
}

void main()
{
    Camera3D camera;

    const int screenWidth = 1920;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "window");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

//  I've been just setting the working directory to the godot project for testing
//  this deals with relative filepaths easier 
//  CHANGE ME 

    ChangeDirectory("d:/godot/maptest/");
    Scene_T *scene = LoadAscii3D("export.ascii3d");


    camera.position = (Vector3){ 10.0f, 4.0f, 10.0f }; 
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };     
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };         
    camera.fovy = 66.0f;
    camera.projection = CAMERA_PERSPECTIVE;             

    HideCursor();

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
       BeginDrawing();
        ClearBackground(DARKGRAY);
        UpdateCamera(&camera,CAMERA_FREE);
        SetMousePosition(GetScreenWidth()/2,GetScreenHeight()/2);
        BeginMode3D(camera);
            DrawGrid(100,20);
            DrawAscii3D(scene);
        EndMode3D();
        DrawFPS(10,10);
        EndDrawing();
    }
    UnLoadAscii3D(scene);
    SetTraceLogCallback(NULL);
    CloseWindow();        // Close window and OpenGL context
    return;
}

