#include <iostream>

//white - no figure
//I - BLUE
//O - YELLOW
//T - PURPLE
//S - GREEN
//Z - RED
//J - MAROON
//L - ORANGE
const int colors[8] = {0xFFFF,0x001F, 0xFFE0, 0x780F, 0x07E0, 0xF800, 0x7800, 0xFD20};
short board[20][10] = {0}; //matrica boja, 0 - 7 indeksi boja

short figuresX[7][4] = {{0,0,0,0}, {0,0,1,1}, {0,1,1,1}, {0,0,1,1}, {0,0,1,1}, {0,1,1,1}, {0,1,1,1}};
short figuresY[7][4] = {{0,1,2,3}, {0,1,0,1}, {1,0,1,2}, {1,2,0,1}, {0,1,1,2}, {0,0,1,2}, {2,0,1,2}};

class Tetromino{
private:
    short X[4];
    short Y[4];
    int rotation;
    int boardX, boardY;
    int width = 3, height = 2;
    int color;
public:
    Tetromino(short x[], short y[], int boardX, int boardY, int rotation){
        copyFigure(x,y);

        this -> boardX = boardX;
        this -> boardY = boardY;

        this -> rotation = rotation ;
    }

   void Rotate(){
       int n;
       if(rotation == 0 || rotation == 2){
        n = height;
       }
       else if(rotation == 1 || rotation == 3){
        n = width - 1;
       }


       for(int i = 0; i < 4; i++){
        int tmp = X[i];
        X[i] = Y[i];
        Y[i] = n - tmp;
       }

       rotation = (rotation + 1) % 4;
   }

   void copyFigure(short x[], short y[]){
       for(int i = 0; i < 4; i++){
        X[i] = x[i];
        Y[i] = y[i];
       }
   }

   /*void MoveDown(){
   }

   void MoveLeft(){
   }

   void MoveRight(){
   }

   bool InCollisionDown(){
   }

   bool InCollisionLeft(){
   }


   bool InCollisionRight(){
   }*/
};


int main()
{
    return 0;
}
