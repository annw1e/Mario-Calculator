#include <windows.h>
#include <stdio.h>

// ======================================================
// SETTINGS
// ======================================================
const int GAME_WIDTH  = 1280;
const int GAME_HEIGHT = 720;

const int TILE = 64;
const int LEVEL_WIDTH  = 90;
const int LEVEL_HEIGHT = 12;

const int MARIO_W = 48;
const int MARIO_H = 64;

float gravity   = 0.8f;
float moveSpeed = 6.0f;
float jumpForce = -20.0f;

// Tile types
// 0 empty
// 1 ground
// 2 castle floor
// 3 lava
// 4 bridge

enum GameState { LEVEL1, CASTLE, WINSCREEN };
GameState gameState = LEVEL1;

// ======================================================
struct Player {
    float x,y;
    float velX,velY;
    bool onGround;
};

struct Goomba {
    float x,y;
    float velX,velY;
    bool alive;
};

struct Bowser {
    float x,y;
    bool alive;
};

Player mario;
Goomba goombas[10];
int goombaCount = 0;
Bowser bowser;

int level[LEVEL_HEIGHT][LEVEL_WIDTH];

bool keys[256] = {0};
float cameraX = 0;

int level1Score = 0;
int castleScore = 0;

char selectedOperator = '+';
bool operatorChosen = false;

int finalResult = 0;

int flagX = 0;
int answerFlagX = 0;

// ======================================================
HBITMAP marioBMP;
HBITMAP goombaBMP;
HBITMAP bowserBMP;

HDC backDC;
HBITMAP backBitmap;

int windowWidth  = GAME_WIDTH;
int windowHeight = GAME_HEIGHT;

// ======================================================
bool solidTile(int tx,int ty)
{
    if(tx < 0 || tx >= LEVEL_WIDTH ||
       ty < 0 || ty >= LEVEL_HEIGHT)
        return false;

    return level[ty][tx] == 1 ||
           level[ty][tx] == 2 ||
           level[ty][tx] == 4;
}

// ======================================================
void resetPlayer(float startX)
{
    mario.x = startX;
    mario.y = 200;
    mario.velX = 0;
    mario.velY = 0;
    mario.onGround = false;
}

// ======================================================
// LEVEL 1
// ======================================================
void buildLevel1()
{
    operatorChosen = false;

    for(int y=0;y<LEVEL_HEIGHT;y++)
        for(int x=0;x<LEVEL_WIDTH;x++)
            level[y][x] = 0;

    for(int x=0;x<LEVEL_WIDTH;x++)
        level[LEVEL_HEIGHT-1][x] = 1;

    level[LEVEL_HEIGHT-1][12] = 0;
    level[LEVEL_HEIGHT-1][13] = 0;

    level[LEVEL_HEIGHT-1][25] = 0;
    level[LEVEL_HEIGHT-1][26] = 0;

    int stairStart = 55;
    for(int step=0; step<6; step++)
        for(int h=0; h<=step; h++)
            level[LEVEL_HEIGHT-1-h][stairStart+step] = 1;

    flagX = (stairStart + 7) * TILE;

    goombaCount = 6;
    for(int i=0;i<goombaCount;i++)
    {
        goombas[i].x = 500 + i*250;
        goombas[i].y = (LEVEL_HEIGHT-2)*TILE;
        goombas[i].velX = (i%2==0)?-2:2;
        goombas[i].velY = 0;
        goombas[i].alive = true;
    }
}

// ======================================================
// CASTLE
// ======================================================
void buildCastle()
{
    for(int y=0;y<LEVEL_HEIGHT;y++)
        for(int x=0;x<LEVEL_WIDTH;x++)
            level[y][x] = 0;

    for(int x=0;x<LEVEL_WIDTH;x++)
        level[LEVEL_HEIGHT-1][x] = 2;

    // lava pits
    for(int x=18;x<20;x++)
        level[LEVEL_HEIGHT-1][x] = 3;

    for(int x=38;x<40;x++)
        level[LEVEL_HEIGHT-1][x] = 3;

    // bridge
    for(int x=60;x<68;x++)
        level[LEVEL_HEIGHT-2][x] = 4;

    bowser.x = 66 * TILE;
    bowser.y = (LEVEL_HEIGHT-3)*TILE;
    bowser.alive = true;

    goombaCount = 3;
    for(int i=0;i<goombaCount;i++)
    {
        goombas[i].x = 600 + i*350;
        goombas[i].y = (LEVEL_HEIGHT-2)*TILE;
        goombas[i].velX = -2;
        goombas[i].velY = 0;
        goombas[i].alive = true;
    }

    answerFlagX = 80 * TILE;
}

// ======================================================
void calculateFinalResult()
{
    if(selectedOperator=='+')
        finalResult = level1Score + castleScore;
    else if(selectedOperator=='-')
        finalResult = level1Score - castleScore;
    else if(selectedOperator=='*')
        finalResult = level1Score * castleScore;
    else
        finalResult = (castleScore!=0)?
                      level1Score/castleScore : 0;
}

// ======================================================
void updatePlayer()
{
    mario.velX = 0;

    if(keys['A']) mario.velX = -moveSpeed;
    if(keys['D']) mario.velX =  moveSpeed;

    if(keys[VK_SPACE] && mario.onGround)
    {
        mario.velY = jumpForce;
        mario.onGround = false;
    }

    mario.velY += gravity;

    mario.x += mario.velX;

    int left   = (int)(mario.x / TILE);
    int right  = (int)((mario.x+MARIO_W-1) / TILE);
    int top    = (int)(mario.y / TILE);
    int bottom = (int)((mario.y+MARIO_H-1) / TILE);

    for(int y=top;y<=bottom;y++)
    {
        if(solidTile(left,y))
            mario.x = (left+1)*TILE;

        if(solidTile(right,y))
            mario.x = right*TILE - MARIO_W;
    }

    mario.y += mario.velY;
    mario.onGround = false;

    left   = (int)(mario.x / TILE);
    right  = (int)((mario.x+MARIO_W-1) / TILE);
    top    = (int)(mario.y / TILE);
    bottom = (int)((mario.y+MARIO_H-1) / TILE);

    for(int x=left;x<=right;x++)
    {
        if(level[bottom][x] == 3)
        {
            resetPlayer(200);
            return;
        }

        if(solidTile(x,bottom))
        {
            mario.y = bottom*TILE - MARIO_H;
            mario.velY = 0;
            mario.onGround = true;
        }

        if(solidTile(x,top))
        {
            mario.y = (top+1)*TILE;
            mario.velY = 0;
        }
    }

    if(mario.y > GAME_HEIGHT)
        resetPlayer(200);

    cameraX = mario.x - GAME_WIDTH/3;
    if(cameraX < 0) cameraX = 0;
}

// ======================================================
void updateGoombas()
{
    for(int i=0;i<goombaCount;i++)
    {
        if(!goombas[i].alive) continue;

        goombas[i].velY += gravity;
        goombas[i].x += goombas[i].velX;
        goombas[i].y += goombas[i].velY;

        int bottom = (int)((goombas[i].y+48)/TILE);

        if(solidTile((int)(goombas[i].x/TILE),bottom))
        {
            goombas[i].y = bottom*TILE - 48;
            goombas[i].velY = 0;
        }

        int front = (int)((goombas[i].x +
                     (goombas[i].velX>0?48:0))/TILE);

        if(!solidTile(front,bottom))
            goombas[i].velX *= -1;
    }
}

// ======================================================
void handleCollisions()
{
    for(int i=0;i<goombaCount;i++)
    {
        if(!goombas[i].alive) continue;

        if(mario.x < goombas[i].x+48 &&
           mario.x+MARIO_W > goombas[i].x &&
           mario.y < goombas[i].y+48 &&
           mario.y+MARIO_H > goombas[i].y)
        {
            if(mario.velY > 0)
            {
                goombas[i].alive=false;
                if(gameState==LEVEL1) level1Score++;
                else castleScore++;
                mario.velY = jumpForce/2;
            }
            else resetPlayer(200);
        }
    }

    if(gameState==CASTLE && bowser.alive)
    {
        if(mario.x < bowser.x+80 &&
           mario.x+MARIO_W > bowser.x &&
           mario.y < bowser.y+80 &&
           mario.y+MARIO_H > bowser.y)
        {
            if(mario.velY > 0)
            {
                bowser.alive = false;
                for(int x=60;x<68;x++)
                    level[LEVEL_HEIGHT-2][x] = 0;
                mario.velY = jumpForce/2;
            }
            else resetPlayer(200);
        }
    }
}

// ======================================================
void checkTransition()
{
    if(gameState==LEVEL1 && mario.x > flagX)
    {
        int section = mario.y / (GAME_HEIGHT/4);
        const char ops[4] = {'+','-','*','/'};
        selectedOperator = ops[section];
        operatorChosen = true;

        gameState = CASTLE;
        buildCastle();
        resetPlayer(200);
    }

    if(gameState==CASTLE && mario.x > answerFlagX)
    {
        calculateFinalResult();
        gameState = WINSCREEN;
    }
}

// ======================================================
void drawBitmap(HDC hdc,HBITMAP bmp,int x,int y,int w,int h)
{
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP old = (HBITMAP)SelectObject(mem,bmp);

    BITMAP bm;
    GetObject(bmp,sizeof(bm),&bm);

    StretchBlt(hdc,x,y,w,h,mem,0,0,
               bm.bmWidth,bm.bmHeight,SRCCOPY);

    SelectObject(mem,old);
    DeleteDC(mem);
}

// ======================================================
void drawGame()
{
    HBRUSH bg = CreateSolidBrush(
        gameState==CASTLE?
        RGB(50,50,50):
        RGB(135,206,235));

    RECT r={0,0,GAME_WIDTH,GAME_HEIGHT};
    FillRect(backDC,&r,bg);
    DeleteObject(bg);

    // tiles
    for(int y=0;y<LEVEL_HEIGHT;y++)
    for(int x=0;x<LEVEL_WIDTH;x++)
    {
        COLORREF c;

        if(level[y][x]==1) c = RGB(139,69,19);
        else if(level[y][x]==2) c = RGB(180,180,180);
        else if(level[y][x]==3) c = RGB(255,0,0);
        else if(level[y][x]==4) c = RGB(120,70,15);
        else continue;

        HBRUSH b = CreateSolidBrush(c);

        RECT r2={
            x*TILE-(int)cameraX,
            y*TILE,
            x*TILE-(int)cameraX+TILE,
            y*TILE+TILE
        };

        FillRect(backDC,&r2,b);
        DeleteObject(b);
    }

    // multi operator flag (level 1)
    if(gameState==LEVEL1)
    {
        int poleX = flagX - (int)cameraX;

        HBRUSH pole = CreateSolidBrush(RGB(230,230,230));
        RECT p = {
            poleX,
            (LEVEL_HEIGHT-9)*TILE,
            poleX+8,
            (LEVEL_HEIGHT-1)*TILE
        };
        FillRect(backDC,&p,pole);
        DeleteObject(pole);

        const char ops[4] = {'+','-','*','/'};
        COLORREF colors[4] = {
            RGB(0,200,0),
            RGB(0,0,255),
            RGB(255,165,0),
            RGB(255,0,0)
        };

        for(int i=0;i<4;i++)
        {
            HBRUSH flagBrush = CreateSolidBrush(colors[i]);

            RECT f = {
                poleX+8,
                (LEVEL_HEIGHT-9+i)*TILE,
                poleX+48,
                (LEVEL_HEIGHT-8+i)*TILE
            };

            FillRect(backDC,&f,flagBrush);
            DeleteObject(flagBrush);

            char text[2] = { ops[i], '\0' };
            SetTextColor(backDC,RGB(255,255,255));
            SetBkMode(backDC,TRANSPARENT);

            TextOut(backDC,
                    poleX+22,
                    (LEVEL_HEIGHT-9+i)*TILE+5,
                    text,1);
        }
    }

    // white answer flag (castle end)
    if(gameState==CASTLE)
    {
        int poleX = answerFlagX - (int)cameraX;

        HBRUSH pole = CreateSolidBrush(RGB(255,255,255));
        RECT p = {
            poleX,
            (LEVEL_HEIGHT-9)*TILE,
            poleX+8,
            (LEVEL_HEIGHT-1)*TILE
        };
        FillRect(backDC,&p,pole);
        DeleteObject(pole);
    }

    // bowser bmp
    if(gameState==CASTLE && bowser.alive)
        drawBitmap(backDC,bowserBMP,
                   (int)(bowser.x-cameraX),
                   (int)bowser.y,
                   80,80);

    for(int i=0;i<goombaCount;i++)
        if(goombas[i].alive)
            drawBitmap(backDC,goombaBMP,
                       (int)(goombas[i].x-cameraX),
                       (int)goombas[i].y,
                       48,48);

    drawBitmap(backDC,marioBMP,
               (int)(mario.x-cameraX),
               (int)mario.y,
               MARIO_W,MARIO_H);

    char text[200];

    if(operatorChosen)
        sprintf(text,"Operator Selected: %c",selectedOperator);
    else
        sprintf(text,"Choose an Operator!");

    SetBkMode(backDC,TRANSPARENT);
    TextOut(backDC,50,50,text,strlen(text));

    if(gameState==WINSCREEN)
    {
        sprintf(text,"RESULT: %d %c %d = %d",
                level1Score,
                selectedOperator,
                castleScore,
                finalResult);

        TextOut(backDC,400,300,text,strlen(text));
    }
}

// ======================================================
void render(HDC hdc)
{
    drawGame();
    StretchBlt(hdc,0,0,
               windowWidth,windowHeight,
               backDC,0,0,
               GAME_WIDTH,GAME_HEIGHT,
               SRCCOPY);
}

// ======================================================
LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,
                            WPARAM wParam,LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
        {
            SetTimer(hwnd,1,16,NULL);

            marioBMP=(HBITMAP)LoadImage(NULL,"mario.bmp",
                IMAGE_BITMAP,0,0,
                LR_LOADFROMFILE|LR_CREATEDIBSECTION);

            goombaBMP=(HBITMAP)LoadImage(NULL,"goomba.bmp",
                IMAGE_BITMAP,0,0,
                LR_LOADFROMFILE|LR_CREATEDIBSECTION);

            bowserBMP=(HBITMAP)LoadImage(NULL,"bowser.bmp",
                IMAGE_BITMAP,0,0,
                LR_LOADFROMFILE|LR_CREATEDIBSECTION);

            HDC hdc=GetDC(hwnd);
            backDC=CreateCompatibleDC(hdc);
            backBitmap=CreateCompatibleBitmap(hdc,
                        GAME_WIDTH,GAME_HEIGHT);
            SelectObject(backDC,backBitmap);
            ReleaseDC(hwnd,hdc);

            buildLevel1();
            resetPlayer(200);
        }
        break;

        case WM_SIZE:
            windowWidth=LOWORD(lParam);
            windowHeight=HIWORD(lParam);
        break;

        case WM_TIMER:
            if(gameState!=WINSCREEN)
            {
                updatePlayer();
                updateGoombas();
                handleCollisions();
                checkTransition();
            }
            InvalidateRect(hwnd,NULL,FALSE);
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc=BeginPaint(hwnd,&ps);
            render(hdc);
            EndPaint(hwnd,&ps);
        }
        break;

        case WM_KEYDOWN: keys[wParam]=true; break;
        case WM_KEYUP:   keys[wParam]=false; break;
        case WM_DESTROY: PostQuitMessage(0); break;
    }

    return DefWindowProc(hwnd,msg,wParam,lParam);
}

// ======================================================
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE,
                   LPSTR,
                   int nCmdShow)
{
    WNDCLASS wc={0};
    wc.lpfnWndProc=WindowProc;
    wc.hInstance=hInstance;
    wc.lpszClassName="MarioCalculator";
    RegisterClass(&wc);

    HWND hwnd=CreateWindow("MarioCalculator",
        "Mario Calculator",
        WS_OVERLAPPEDWINDOW,
        100,100,GAME_WIDTH,GAME_HEIGHT,
        NULL,NULL,hInstance,NULL);

    ShowWindow(hwnd,nCmdShow);

    MSG msg;
    while(GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
