#include <stdio.h>
#include <string.h>
#include <raylib.h>         
#include <raymath.h>

//embedded animations
#include "idle_anim.h"
#include "run_anim.h"
#include "slash_anim.h"
#include "death_anim.h"

//••••••••••••••••••••••••••••••••••••••••••••••••<STRUCTURES/ENUMS>•••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••
typedef struct Animation{
    Texture2D spriteSheet;
    int frameCount;
    float frameDuration;
}Animation;

typedef struct Enemy{
    Vector2 position;
    float speed;
    float size;
    int level;
    int keyCode[11];
    char keyName[11];
    Color color;
    bool focused;
    bool dead;
}Enemy;

typedef struct Player{      
    Vector2 position;
    float speed;
    float size;
    float zoneRadius;
    Color color;
    bool dead;
    
    int focusedEnemiesIndex[10];
    int focusedCount;
    float slashInterval;
    float time;
    bool slashing;
    
    int currentFrame;
    float frameTime;
    Animation currentAnim;
    bool isFacingRight;
    
}Player;

typedef struct Spawner{
    Vector2 position;
    float interval;
    float spawnTimer;
    Enemy enemies[100];
    float angle;
    float speed;
    float radius;
    float difficultyTimer;
    float enemySpeed;
}Spawner;

typedef struct HighScore{
    char name[11];
    int score;
}HighScore;    

enum scene{
    mainMenu,
    gamePlay,
    gameOver,
    leaderBoard
};

//••••••••••••••••••••••••••••••••••••••••••••••••<VARIABLES>•••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••
//Initialization
int screenWidth = 1280;
int screenHeight = 720;

//Gameplay
Player player = { 0 };
Camera2D camera = { 0 };
Spawner spawner = { 0 };
int slashEnemy;

//score and stats calculation
int Score = 0;
float timeToType = 0;       
int charactersTyped = 0;
float timeSurvived = 0;     

//scene transitioning
enum scene currentScene = gamePlay;

//leader board 
HighScore highScores[10];
bool askForName = false;
int newHighScoreIndex;
int letterCount = 0;

//player animations
Animation idleAnim;
Animation runAnim;
Animation slashAnim;
Animation deathAnim;


//••••••••••••••••••••••••••••••••••••••••••••••••<FUNCTIONS>•••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••

//==================================================MYSC FUNCTIONS================================================================
void drawLine(Vector2 pointA, Vector2 pointB, float lineWidth, Color color){
    Vector2 direction = Vector2Subtract(pointB, pointA);
    direction = Vector2Normalize(direction);
    
    Vector2 points[] = {
        {pointA.x + lineWidth * direction.y, pointA.y - lineWidth * direction.x},
        {pointA.x - lineWidth * direction.y, pointA.y + lineWidth * direction.x},
        {pointB.x - lineWidth * direction.y, pointB.y + lineWidth * direction.x},
        {pointB.x + lineWidth * direction.y, pointB.y - lineWidth * direction.x}
        
        
    };
    
    DrawTriangleFan(points, 4, color);
}

void readFile(HighScore highScores[]){
	FILE *readFile = fopen("leaderBoard.txt", "r");
    if(readFile == NULL){
        FILE *createFile = fopen("leaderBoard.txt", "w");       //create new file if not found
        fclose(createFile);
        return;
    }
	for(int i = 0; i < 10; i++){
		if(!feof(readFile)){
			fscanf(readFile, "Name: %s Score: %d \n", &highScores[i].name, &highScores[i].score);
		}else{
			strcpy(highScores[i].name, "");
			highScores[i].score = 0;
		}
	}
	fclose(readFile);
}

void saveFile(HighScore highScores[]){
	FILE *writeFile = fopen("leaderBoard.txt", "w");
	for(int i = 0; i < 10; i++){
		if(highScores[i].score > 0){
			fprintf(writeFile, "Name: %-11s Score: %d \n", highScores[i].name, highScores[i].score);
		}
	}
	fclose(writeFile);
}

void makeSpace(int index, HighScore highScores[]){
	for(int i = 9; i > index; i--){
		strcpy(highScores[i].name, highScores[i - 1].name);
		highScores[i].score = highScores[i - 1].score;
	}
}

void nextScene(int score, HighScore highScores[]){
	for(int i = 0; i < 10; i++){
		if(score > highScores[i].score){
			makeSpace(i, highScores);
			strcpy(highScores[i].name, "");
			highScores[i].score = score;
			
            askForName = true;
            newHighScoreIndex = i;
            letterCount = 0;
            
			currentScene = leaderBoard;
            return;
		}
	}
    currentScene = gameOver;
}

void loadAnimations(){
    idleAnim.frameCount = 13;
    idleAnim.frameDuration = 0.1;
    runAnim.frameCount = 8;
    runAnim.frameDuration = 0.1;
    slashAnim.frameCount = 18;
    slashAnim.frameDuration = 0.1;
    deathAnim.frameCount = 23;
    deathAnim.frameDuration = 0.1;
    
    //load textures for player animations
    Image img = {0};
    img.format = IDLE_ANIM_FORMAT;
    img.width = IDLE_ANIM_WIDTH;
    img.height = IDLE_ANIM_HEIGHT;
    img.data = IDLE_ANIM_DATA;
    img.mipmaps = 1;
    idleAnim.spriteSheet = LoadTextureFromImage(img);
    
    img.format = RUN_ANIM_FORMAT;
    img.width = RUN_ANIM_WIDTH;
    img.height = RUN_ANIM_HEIGHT;
    img.data = RUN_ANIM_DATA;
    runAnim.spriteSheet = LoadTextureFromImage(img);
    
    img.format = SLASH_ANIM_FORMAT;
    img.width = SLASH_ANIM_WIDTH;
    img.height = SLASH_ANIM_HEIGHT;
    img.data = SLASH_ANIM_DATA;
    slashAnim.spriteSheet = LoadTextureFromImage(img);
    
    img.format = DEATH_ANIM_FORMAT;
    img.width = DEATH_ANIM_WIDTH;
    img.height = DEATH_ANIM_HEIGHT;
    img.data = DEATH_ANIM_DATA;
    deathAnim.spriteSheet = LoadTextureFromImage(img);
}

//==================================================INITIALIZATION================================================================
void resetGame(){
    player.position = (Vector2) {screenWidth / 2, screenHeight / 2};
    player.size = 25;
    player.speed = 200;
    player.zoneRadius = 300;
    player.color = WHITE;
    player.slashInterval = 0.9  ;
    player.slashing = false;
    for(int i = 0; i < 10; i++){ player.focusedEnemiesIndex[i] = -1; }
    player.focusedCount = 0;
    player.dead = false;
    
    player.currentAnim = idleAnim;
    player.currentFrame = 0;
    player.frameTime = 0;
    player.isFacingRight = true;
    
    camera.target = player.position;
    camera.offset = (Vector2) { screenWidth / 2.0, screenHeight / 2.0 };
    camera.zoom = 0.75;
    
    spawner.position = (Vector2) {screenWidth / 2, screenHeight / 2};
    spawner.speed = 0.02;
    spawner.radius = 1000;
    spawner.interval = 1;
    for(int i = 0; i < 100; i++){ spawner.enemies[i].dead = true; }
    spawner.enemySpeed = 150;
    
    Score = 0;
    timeSurvived = 0;
    timeToType = 0;
    charactersTyped = 0;
    
    currentScene = gamePlay;
}

void initializeGame(){
    InitWindow(screenWidth, screenHeight, "Platformer Test");
    SetTargetFPS(120);
    
    for(int i = 0; i < 10; i++){
        strcpy(highScores[i].name, "");
        highScores[i].score = 0;
    }
    readFile(highScores);
    
    loadAnimations();
    
    resetGame();
}

//==================================================GAMEPLAY================================================================
void playerMovement(){
    bool isMoving = false;      //for animation
    if(!player.slashing && !player.dead){
        if(IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)){
            player.position.x -= player.speed * GetFrameTime();
            player.isFacingRight = false;
            isMoving = true;
        }
        if(IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)){
            player.position.x += player.speed * GetFrameTime();
            player.isFacingRight = true;
            isMoving = true;
        }
        if(IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)){
            player.position.y -= player.speed * GetFrameTime();
            isMoving = true;
        }
        if(IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)){
            player.position.y += player.speed * GetFrameTime();
            isMoving = true;
        }
    }
    
    //animate player
    if(!player.slashing){
        if(isMoving){
            player.currentAnim = runAnim;
        }else{
            player.currentAnim = idleAnim;
        }   
    }
}

void cameraFollow() {
    camera.offset = (Vector2) { screenWidth / 2.0, screenHeight / 2.0 };
    Vector2 camDirection = Vector2Subtract(player.position, camera.target);
    float length = Vector2Length(camDirection);
    if(length > 5){
        camDirection = Vector2Normalize(camDirection);
        camera.target.x += camDirection.x * length * GetFrameTime();
        camera.target.y += camDirection.y * length * GetFrameTime();
    }
    
    //adjust camera zoom
    if(IsKeyDown(KEY_KP_ADD)){
        camera.zoom += GetFrameTime();
    }else if(IsKeyDown(KEY_KP_SUBTRACT)){
        camera.zoom -= GetFrameTime();
    }
}

void handleSpawner(){
    //spawner movement
    spawner.position.x = player.position.x + cos(spawner.angle) * spawner.radius;
    spawner.position.y = player.position.y + sin(spawner.angle) * spawner.radius;
    spawner.angle += spawner.speed;
    
    //spawn enemies
    spawner.spawnTimer += GetFrameTime();
    if(spawner.spawnTimer > spawner.interval){
        for(int i = 0; i < 100; i++){
            if(spawner.enemies[i].dead == true){
                spawner.enemies[i].position = spawner.position;
                spawner.enemies[i].size = 20;
                spawner.enemies[i].level = 1;
                spawner.enemies[i].speed = GetRandomValue(spawner.enemySpeed - 100, spawner.enemySpeed);
                spawner.enemies[i].color = RED;
                spawner.enemies[i].focused = false;
                spawner.enemies[i].dead = false;
                
                int rng = GetRandomValue(0, 25);
                spawner.enemies[i].keyCode[0] = KEY_A + rng;
                spawner.enemies[i].keyName[0] = 'A' + rng;
                break;
            }
        }
        spawner.spawnTimer = 0;
     }
}

void handleEnemyMovement(){
    //bool for checking if at least one enemy is in player's range
    bool isEnemyInRange = false;
                
    //enemy follow + focus enemies
    for(int i = 0; i < 100; i++){
        if(spawner.enemies[i].dead == false){
            //enemy follow
            if(!player.slashing && !player.dead){
                Vector2 direction = Vector2Subtract(player.position, spawner.enemies[i].position);
                direction = Vector2Normalize(direction);
                spawner.enemies[i].position.x += direction.x * spawner.enemies[i].speed * GetFrameTime();
                spawner.enemies[i].position.y += direction.y * spawner.enemies[i].speed * GetFrameTime();
            }
            
                
            //focus enemies
            float distance = Vector2Distance(player.position, spawner.enemies[i].position);
            if(distance <= player.zoneRadius + spawner.enemies[i].size * spawner.enemies[i].level){
                if(!isEnemyInRange){
                    isEnemyInRange = true;      //enemy is in range
                }  
                if(IsKeyPressed(spawner.enemies[i].keyCode[spawner.enemies[i].level - 1]) && !IsKeyDown(KEY_LEFT_SHIFT) && player.focusedCount < 10 && !player.slashing){    
                    if(spawner.enemies[i].level == 1){
                        //focus
                        int flag = 0;
                        for(int j = 0; j < player.focusedCount; j++){ if(player.focusedEnemiesIndex[j] == i) flag = 1; }        //check if enemy is already focused
                        if(flag == 0){                                                                                          //if enemy is not already focused
                            player.focusedEnemiesIndex[player.focusedCount] = i;
                            spawner.enemies[i].color = MAGENTA;
                            spawner.enemies[i].focused = true;
                            player.focusedCount++;
                            //when player has reached focus limit, turn all enemies to white indicating that limit has been reached
                            if(player.focusedCount == 10){
                                for(int j = 0; j < player.focusedCount; j++){
                                    spawner.enemies[player.focusedEnemiesIndex[j]].color = WHITE;
                                    spawner.enemies[player.focusedEnemiesIndex[j]].color.a = 225;
                                }
                            }
                        }
                    }else{
                        //reduce enemy size
                        spawner.enemies[i].keyCode[spawner.enemies[i].level - 1] = 0;
                        spawner.enemies[i].keyName[spawner.enemies[i].level - 1] = '\0';
                        spawner.enemies[i].level--;
                    }
                    charactersTyped++;
                
                }else if(distance < player.size + spawner.enemies[i].size * spawner.enemies[i].level && !player.slashing){
                    //player.color.a = 0;
                    //nextScene(Score, highScores);   //player has died so proceed to the next scene (i.e. leaderBoard/gameOver)
                    player.currentAnim = deathAnim;
                    player.dead = true;
                }
            }
        }
    }
                
    //increase time had to type characters for calculating typing speed
    if(isEnemyInRange){
        timeToType += GetFrameTime();
    }
    //increase time survived
    timeSurvived += GetFrameTime();
}

void slashEnemies() {
    // Trigger slashing enemies
    if (IsKeyPressed(KEY_SPACE) && !player.slashing && player.focusedCount > 0) {
        player.slashing = true;
        player.time = player.slashInterval; // First slash is instant
        
        // Animate slashing
        player.currentAnim = slashAnim;
        player.currentFrame = 0;
        player.frameTime = 0;
    }
    
    // Slash enemies if slashing is triggered (slashing is true)
    if (player.slashing) {
        player.time += GetFrameTime(); 

        if (player.time >= player.slashInterval) { 
            for (int i = 0; i < 10; i++) {
                if (player.focusedEnemiesIndex[i] != -1) {
                    //bring the player next to the enemy for slashing
                    Vector2 offset = { player.isFacingRight? -75 : 75 , 0 };
                    player.position = Vector2Add(spawner.enemies[player.focusedEnemiesIndex[i]].position, offset);
                    
                    //reset animation
                    player.currentFrame = 0;
                    player.frameTime = 0;
                    
                    //reset the timer and store the enemy index that is going to be slashed
                    player.time = 0;
                    slashEnemy = i;
                    
                    break; 
                }
            }
        }
        
        // If slash animation is complete (when currentFrame exceeds max frame), finish the slash
        if (player.currentFrame >= 8) {
            
            if (!spawner.enemies[player.focusedEnemiesIndex[slashEnemy]].dead) {
                //kill the enemy
                spawner.enemies[player.focusedEnemiesIndex[slashEnemy]].dead = true;
                player.focusedEnemiesIndex[slashEnemy] = -1; 
                player.focusedCount--; 
                
                //increment score
                Score++;
                
                //reset the animation
                player.currentFrame = 0;
                player.frameTime = 0;
            }
            
            // Stop slashing when all enemies have been slashed
            if (player.focusedCount <= 0) {
                player.slashing = false; 
                player.currentFrame = 0;
                player.frameTime = 0;
            }
        }
    }
}

void mergeEnemies(){
    //merge enemies on collision
    for(int i = 0; i < 100; i++){
        if(spawner.enemies[i].dead == false && spawner.enemies[i].focused != true){
            for(int j = i + 1; j < 100; j++){
                if(CheckCollisionCircles(spawner.enemies[i].position, spawner.enemies[i].size * spawner.enemies[i].level, spawner.enemies[j].position, spawner.enemies[j].size * spawner.enemies[j].level) && spawner.enemies[j].dead != true && spawner.enemies[j].focused != true){  
                    if(spawner.enemies[i].level >= spawner.enemies[j].level){
                        if(spawner.enemies[i].level < 10){
                            spawner.enemies[i].keyCode[spawner.enemies[i].level] = spawner.enemies[j].keyCode[spawner.enemies[j].level - 1];
                            spawner.enemies[i].keyName[spawner.enemies[i].level] = spawner.enemies[j].keyCode[spawner.enemies[j].level - 1];
                            spawner.enemies[i].level++;
                        }
                        spawner.enemies[j].dead = true;
                    }else{
                        if(spawner.enemies[j].level < 10){
                            spawner.enemies[j].keyCode[spawner.enemies[j].level] = spawner.enemies[i].keyCode[spawner.enemies[i].level - 1];
                            spawner.enemies[j].keyName[spawner.enemies[j].level] = spawner.enemies[i].keyCode[spawner.enemies[i].level - 1];
                            spawner.enemies[j].level++;
                        }
                        spawner.enemies[i].dead = true;
                    }
                }
            }
        }
    }
}

void increaseDifficulty(){
    spawner.difficultyTimer += GetFrameTime();
    if(spawner.difficultyTimer > 15){
        if(spawner.interval > 0.5){
            spawner.interval -= 0.05;
        }
        spawner.enemySpeed += 5;
        spawner.difficultyTimer = 0;
    }
}

void animatePlayer(){
    player.frameTime += GetFrameTime();
    if(player.frameTime >= player.currentAnim.frameDuration){
        player.currentFrame = (player.currentFrame + 1) % player.currentAnim.frameCount;
        player.frameTime = 0;
    }
    
    Vector2 offset;
    Rectangle frameRec = {player.currentFrame * 280, 0, 280, player.currentAnim.spriteSheet.height};
    if(!player.isFacingRight){
        frameRec.width = -frameRec.width;
        offset = Vector2Add((Vector2){-190, -150}, player.position);
    }else{
        offset = Vector2Add((Vector2){-90, -150}, player.position);
    }
    
    if(player.dead && player.currentFrame >= 22){
        nextScene(Score, highScores);
    }
    
    DrawTextureRec(player.currentAnim.spriteSheet, frameRec, offset, WHITE);
}

void drawEverything(){
    BeginDrawing();
    BeginMode2D(camera);
        
    ClearBackground(BLACK);
        
    //draw enemies
    for(int i = 0; i < 100; i++){
        if(spawner.enemies[i].dead == false){
            int enemyLevel = spawner.enemies[i].level;
            DrawCircle((int) spawner.enemies[i].position.x, (int) spawner.enemies[i].position.y, spawner.enemies[i].size * enemyLevel, spawner.enemies[i].color);
            char enemyChar[2] = { spawner.enemies[i].keyName[enemyLevel - 1], '\0' };
            DrawText(enemyChar, spawner.enemies[i].position.x - 6 * enemyLevel, spawner.enemies[i].position.y - 9 * enemyLevel, spawner.enemies[i].size * enemyLevel, WHITE);
        }
    }
        
    //draw spawner
    //DrawCircle((int) spawner.position.x, (int) spawner.position.y, 10 , YELLOW);
        
    //draw focused path     tho im still not satisfied with this
    if(player.focusedCount > 0 && !player.slashing){
        Color focusedColor = spawner.enemies[player.focusedEnemiesIndex[0]].color;
        focusedColor.a = 40;
        drawLine(player.position, spawner.enemies[player.focusedEnemiesIndex[0]].position, 5, focusedColor);
            
        for(int i = 0; i < player.focusedCount - 1; i++){  //later try comparing with -1 as did in slashing
            drawLine(spawner.enemies[player.focusedEnemiesIndex[i]].position, spawner.enemies[player.focusedEnemiesIndex[i + 1]].position, 5, focusedColor);
        }    
    }
        
    //draw player
    Color zoneColor = player.color;
    zoneColor.a = 40;
    DrawCircle(player.position.x, player.position.y, player.zoneRadius, zoneColor);
    animatePlayer();
    
    EndMode2D();
    
    //show score on the screen
    DrawText(TextFormat("Score = %d", Score), 10, 10, 30, WHITE);
    
    EndDrawing();
}

//==================================================LEADERBOARD================================================================
void displayLeaderBoard(){
    DrawRectangle(screenWidth / 4, screenHeight / 14, screenWidth / 2, screenHeight / 1.15, WHITE);
    DrawRectangle(screenWidth / 4 + 5, screenHeight / 14 + 5, screenWidth / 2 - 10, screenHeight / 1.15 - 10, BLACK);
                
    DrawText("Leader Board", screenWidth / 2.7, screenHeight / 3.5 - 135, 50, WHITE);
    for(int i = 0; i < 10; i++){
        if(highScores[i].score > 0){
            DrawText(TextFormat("%d.", i + 1), screenWidth / 3.5, screenHeight / 4.85 + 45 * i, 35, WHITE);
            DrawText(TextFormat("%s", highScores[i].name), screenWidth / 3.5 + 100, screenHeight / 4.85 + 45 * i, 35, WHITE);
            DrawText(TextFormat("%d", highScores[i].score), screenWidth / 3.5 + 500, screenHeight / 4.85 + 45 * i, 35, WHITE);
            
        }else{
            DrawText(TextFormat("%d.", i + 1), screenWidth / 3.5, screenHeight / 4.85 + 45 * i, 35, WHITE);
            DrawText("__________", screenWidth / 3.5 + 100, screenHeight / 4.85 + 45 * i, 35, WHITE);
            DrawText("0", screenWidth / 3.5 + 500, screenHeight / 4.85 + 45 * i, 35, WHITE);
        }
    }
}

void addNewName(){
    if(askForName){
        int key = GetCharPressed();
        if(key >= 33 && key <= 125 && letterCount < 10){   //33 - 125 is a range of diff char on the key board
            highScores[newHighScoreIndex].name[letterCount] = (char)key;
            highScores[newHighScoreIndex].name[letterCount + 1] = '\0';
            letterCount++;
        }else if(IsKeyPressed(KEY_ENTER)){
            askForName = false;
            saveFile(highScores);
        }else if(IsKeyPressed(KEY_BACKSPACE) && letterCount > 0){
            letterCount--;
            highScores[newHighScoreIndex].name[letterCount] = '\0';
        }
        DrawText("(Press Enter to save...)", screenWidth / 2.7, screenHeight / 3.5 + 415, 25, GRAY);
    }else {
        if(GetCharPressed() != NULL || IsKeyPressed(KEY_ENTER)){        //!= Null wali condition ke baghair check krna
            saveFile(highScores);
            currentScene = gameOver;
        }
        DrawText("(Press Enter to continue...)", screenWidth / 2.7, screenHeight / 3.5 + 415, 25, WHITE);
    }
}


//==================================================GAME OVER================================================================
void gameOverFn(){
    BeginDrawing();
                
    ClearBackground(BLACK);
                
    DrawRectangle(screenWidth / 4, screenHeight / 4, screenWidth / 2, screenHeight / 2, WHITE);
    DrawRectangle(screenWidth / 4 + 5, screenHeight / 4 + 5, screenWidth / 2 - 10, screenHeight / 2 - 10, BLACK);
    
    DrawText("GAME OVER!", screenWidth / 2.7, screenHeight / 3.5, 50, WHITE);
    DrawText(TextFormat("Score = %d", Score), screenWidth / 3.5, screenHeight / 2.55, 35, WHITE);
    DrawText(TextFormat("Typing Speed = %.2f cpm", charactersTyped / timeToType * 60), screenWidth / 3.5, screenHeight / 2.55 + 50, 35, WHITE);
    DrawText(TextFormat("Time Survived = %.2fs", timeSurvived), screenWidth / 3.5, screenHeight / 2.55 + 100, 35, WHITE);
                
    Rectangle retryButton = { screenWidth / 2.3, screenHeight / 2 + 100, 150, 50};
    DrawRectangleRec(retryButton, WHITE);
    DrawRectangle(retryButton.x + 2.5, retryButton.y + 2.5, retryButton.width - 5, retryButton.height - 5, BLACK);
    DrawText("RETRY", retryButton.x + (retryButton.width / 6), retryButton.y + (retryButton.height / 5), 30, WHITE);
    
    //restart game when the retry button is pressed
    if(CheckCollisionPointRec(GetMousePosition(), retryButton) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
        resetGame(); 
    }
                
    EndDrawing();
}


//••••••••••••••••••••••••••••••••••••••••••••••••<THE MAIN FUNCTION>•••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••
int main(void)
{
    initializeGame();
    
    while (!WindowShouldClose())
    {
        
        //toggle full screen
        if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_F)){
            ToggleFullscreen();
        }
        
        switch(currentScene){
            case gamePlay:
            
                //-----------------------------------CALCULATION PHASE--------------------------------------------------
                //Calculate everything and handle Inputs; e.g. the position of each object (player/camera/enemy/spawner)
               
                playerMovement();
                
                cameraFollow();
                
                handleSpawner();
                
                handleEnemyMovement();
                
                slashEnemies();
                
                mergeEnemies();
                
                increaseDifficulty();
                
                //---------------------------------------OUTPUT PHASE------------------------------------------------
                //After calculation, output or draw everything on the screen according to their position/other values
                
                drawEverything();
                
                break;
                
            case leaderBoard:
                BeginDrawing();
                
                ClearBackground(BLACK);
                
                //display leaderBoard
                displayLeaderBoard();
                
                //ask for new name
                addNewName();
                
                EndDrawing();
                break;
                
            case gameOver:
                gameOverFn();
                break;
        }
    }

    CloseWindow();

    return 0;
}
