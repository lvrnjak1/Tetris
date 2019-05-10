#include "mbed.h"
#include "SPI_TFT_ILI9341.h"
#include "Arial12x12.h"

#define dp23 P0_0

//deklaracija display-a
SPI_TFT_ILI9341 display(dp2, dp1, dp6, dp24, dp23, dp25, "TFT");
//ide redom mosi, miso, sclk, cs, reset, dc

//analogni ulazi za joystick
AnalogIn VRx(dp11);
AnalogIn VRy(dp10);
//taster na joysticku za rotaciju
InterruptIn rotateBtn(dp9);

void InitializeDisplay()
{
    //ovdje inicijalizujemo display
    display.claim(stdout);
    display.set_orientation(2); // 2 ili 0, zavisi kako okrenemo display, provjerit ćemo na labu kako nam je najlakše povezat
    display.background(White); //bijela pozadina
    display.foreground(Black); //crni tekst
    display.set_font((unsigned char*) Arial12x12);
}

//white - no figure
//I - BLUE
//O - YELLOW
//T - PURPLE
//S - GREEN
//Z - RED
//J - MAROON
//L - ORANGE
const int colors[8] = {0xFFFF,0x001F, 0xFFE0, 0x780F, 0x07E0, 0xF800, 0x7800, 0xFD20};
const short DIMENSION = 16;
short board[20][10] = {0}; //matrica boja, 0 - 7 indeksi boja

short figuresX[7][4] = {{0,0,0,0}, {0,0,1,1}, {0,1,1,1}, {1,1,0,0}, {0,1,0,1}, {0,1,2,2}, {1,1,1,0}};
short figuresY[7][4] = {{0,1,2,3}, {1,0,0,1}, {1,1,2,0}, {0,1,1,2}, {0,1,1,2}, {1,1,1,0}, {0,1,2,2}};

void copyCoordinates(short X[], short Y[], short index)
{
    //umjesto one prošle metode za kopiranje, ova prima index, čisto da nam olakša život
    for(int i = 0; i < 4; i++) {
        X[i] = figuresX[index][i];
        Y[i] = figuresY[index][i];
    }
}

class Tetromino{
private:
    short X[4];
    short Y[4];
    short boardX, boardY;
    int color;
public:
    Tetromino(short color) {
        this.color = colors[color];
        boardX = 0;
        boardY = 4; //3,4 ili 5 najbolje da vidimo kad imamo display
        copyCoordinates(X, Y, color);
    }
    
    void Rotate(){
       short pivotX = X[1], pivotY = Y[1];
        //prvi elemnti u matrici su pivoti oko koje rotiramo
       for(int i = 0; i < 4; i++) {
           short tmp = X[i], tmp2 = Y[i];
           X[i] = pivotX + pivotY - tmp2;
           Y[i] = tmp + pivotX - pivotY;
           //morao sam promijeniti ono tvoje, jer sam našao pravilno kako se rotira
           //ne može ostati X[i] = Y[i] kao kod tebe jer je to clockwise, a ovo counter-clockwise
           //provjerio sam na nekoliko slučajeva i radi ali provjeri i ti
       }
   }

    void DrawFigure() {
        for(int i = 0; i < 4; i++) {
            //display je deklarisani display logy iz biblioteke one 
            //računamo gornji lijevi pixel po x i y
            //donji desni dobijemo kad dodamo DIMENZIJU
            //stavio sam 16 za početak, možemo se opet skontati na labu
            //ovo pretpostavlja da nema margina, mogu se lagano dodati uz neku konstantu kao offset
            int upperLeftX = (boardX + X[i]) * DIMENSION, uppperLeftY = (boardY + Y[i]) * DIMENSION;
            display.fillrect(upperLeftX, upperLeftY, upperLeftX + DIMENSION, upperLeftY + DIMENSION, color);
            //ovo boji granice blokova u crno, možemo skloniti ako ti se ne sviđa
            display.rect(upperLeftX, upperLeftY, upperLeftX + DIMENSION, upperLeftY + DIMENSION, Black);
        }
    }
    
    void DeleteFigure() {
        for(int i = 0; i < 4; i++) {
            //ista logika kao u DrawFigure, samo popunjavamo sve blokove sa bijelim pravougaonicima
            int upperLeftX = (boardX + X[i]) * DIMENSION, uppperLeftY = (boardY + Y[i]) * DIMENSION;
            display.fillrect(upperLeftX, upperLeftY, upperLeftX + DIMENSION, upperLeftY + DIMENSION, White);
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
 
/*VAŽNO: nisam htio da izvršim histerezu joystick-a još jer
to zahtjeva 10+ float varijabli da se definišu granice napona. Vidjet ćemo ako nam ostane memorije, da to uradimo. */

void RotateFigure()
{
    //ovdje pozivamo funkciju da provjerimo da li se može rotirati(zbog granica itd)
    //ako može, pozovemo funkciju Rotate nad instancom klase Figura
    //ovo se veže za InterruptIn
}

int main()
{
    rotateBtn.mode(PullUp); //mora se aktivirati pull up otpornik na tasteru joystick-a
    rotateBtn.rise(&RotateFigure); //na uzlaznu ivicu
    InitializeDisplay(); //ovdje se uključi display
    while(1) {
        //vidjet ćemo ide li išta u while
    }
    return 0;
}
