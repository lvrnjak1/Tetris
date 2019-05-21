#include "mbed.h"
#include "SPI_TFT_ILI9341.h"
#include "Arial12x12.h"

#define dp23 P0_0

//deklaracija display-a
SPI_TFT_ILI9341 display(dp2, dp1, dp6, dp24, dp23, dp25, "TFT");

//analogni ulazi za joystick
AnalogIn VRx(dp11);
AnalogIn VRy(dp10);
//taster na joysticku za rotaciju
InterruptIn taster(dp9);

//ticker za spustanje figure
//timer za debouncing tastera na joysticku
Ticker game, menu;
Timer debounceTaster;


unsigned char level = 0; //mora biti tipa usigned char jer inače se može desiti da level bude manji od 0, a i da ne trošimo memoriju
const float delays[3] = {1, 0.7, 0.4}; //svakih koliko se spusti jedan red, ovo provjeriti da li je presporo ili prebrzo, ovisi o levelu
char leftBoundary = 1, rightBoundary = 5, downBoundary = 1, upBoundary = 5;// sada je ovo tipa char
unsigned int score = 0; //stavio sam ovo unsigned int za veći opseg, mada je jako teško da se i int premaši, ali nmvz
bool firstTime = true; //ako je prvi put, figura se crta u Tickeru
bool gameStarted = false;


//white - no figure
//I - BLUE
//O - YELLOW
//T - PURPLE
//S - GREEN
//Z - RED
//J - MAROON
//L - ORANGE
const int colors[8] = {White, Blue, Yellow, Purple, Green, Red, Maroon, Orange};
const short DIMENSION = 16;
short board[20][10] = {0}; //matrica boja, 0 - 7 indeksi boja

short figuresX[7][4] = {{0,0,0,0}, {0,0,1,1}, {0,1,1,1}, {1,1,0,0}, {0,1,0,1}, {0,1,2,2}, {1,1,1,0}};
short figuresY[7][4] = {{0,1,2,3}, {1,0,0,1}, {1,1,2,0}, {0,1,1,2}, {0,1,1,2}, {1,1,1,0}, {0,1,2,2}};

void Init() {
   //ovo su zajedničke osobine za sve prikaze na display-u
   //nikad se ne mijenjaju i pozvat ćemo je jednom prije petlje
    display.claim(stdout);
    display.set_orientation(2); // 2 ili 0, zavisi kako okrenemo display, provjerit ćemo na labu kako nam je najlakše povezat 
    display.set_font((unsigned char*) Arial12x12);
}


//funkcija za crtanje cursora za odabir levela
void DrawCursor(int color, unsigned char lev) {
    display.fillrect(60, (lev + 1) * 100 - 50, 72, (lev + 1) * 100 - 50 + 12,  color);
}

void ShowLevelMenu() {
    //ovdje inicijalizujemo display
    display.cls(); // brišemo prethodno
    display.background(Black);
    display.foreground(White);
    display.locate(80, 50);
    printf("LEVEL 1");
    display.locate(80, 150);
    printf("LEVEL 2");
    display.locate(80, 250);
    printf("LEVEL 3");
    DrawCursor(White, level);
}

void ReadJoystickForLevel(){
    unsigned char old = level;
    if(VRy < downBoundary / 6.0){
        downBoundary = 2;
        level = (level + 1) % 3;
    }
    else if(VRy > upBoundary / 6.0){
        upBoundary = 4;
        (level == 0) ? level = 2 : level--; //ne radi ona prethodna varijanta jer % vraća i negastivni rezultat
     //to što ne koristimo unsigned tip ne pomaže jer će doći do overflow-a
    }
    else {
        downBoundary = 1;
        upBoundary = 5;
    }
    DrawCursor(Black, old); //na prethodni level popunimo bojom pozadine
    DrawCursor(White, level); //na novi level popunimo bijelom bojom - pozadina je crna
   //koristio sam fillrect, jer njega svakako moramo koristiti, jer možda budemo morali da brišemo fillcircle iz biblioteke
}

void ShowScore() {
    //pomocna funkcija za prikazivanje score-a
    display.fillrect(165, 20, 235, 50, White); //popunimo pravugaonik da obrišemo stari score
    display.locate(200, 35); //valjda je na sredini pravougaonika
    printf("%d", score);
}

void InitializeDisplay()
{
    display.cls(); // brišemo ShowLevelMenu
    display.background(White);
    display.foreground(Black);
    display.fillrect(0, 0, 160, 320, White);
    display.fillrect(160, 0, 240, 320, Black); //dio za prikazivanje rezultata će biti crni pravougaonik, a tabla je bijeli
    ShowScore();
}




void copyCoordinates(short X[], short Y[], unsigned char index)
{
    //umjesto one prošle metode za kopiranje, ova prima index, čisto da nam olakša život
    for(int i = 0; i < 4; i++) {
        X[i] = figuresX[index][i];
        Y[i] = figuresY[index][i];
    }
}

bool BottomEdge(int x){
    return x == 19;
}

bool LeftEdge(int y){
    return y  == 0;
}

bool RightEdge(int y){
    return y == 9;
}

bool OutOfBounds(int x, int y){
    return x < 0 || x > 9 || y > 19;
}


class Tetromino{
private:
    short X[4];
    short Y[4];
    short boardX, boardY;
    unsigned char colorIndex;//dodao sam colorIndex zasad, jer nema drugog načina da popunimo matricu sa indeksima boja
    //ovo je najbezbolnija varijanta što se memorije tiče
public:
    Tetromino(){
        unsigned char r = rand() % 7 + 1;
        Initialize(r);
        score = r;
    }

    Tetromino(unsigned char colorIndex) {
        Initialize(colorIndex);
    }

    void Initialize(unsigned char colorIndex) {
        Tetromino::colorIndex = colorIndex;
        boardX = 0;
        boardY = 4; //3,4 ili 5 najbolje da vidimo kad imamo display
        copyCoordinates(X, Y, colorIndex);
    }
    
    void Rotate(){
       short pivotX = X[1], pivotY = Y[1];
        //prvi elemnti u matrici su pivoti oko koje rotiramo
        
       short newX[4]; //pozicije blokova nakon rotiranja ako sve bude ok
       short newY[4];

       for(int i = 0; i < 4; i++){
           short tmp = X[i], tmp2 = Y[i];
           newX[i] = pivotX + pivotY - tmp2;
           newY[i] = tmp + pivotX - pivotY;

           if(OutOfBounds(newX[i], newY[i])
              || (board[boardX + newX[i]][boardY + newX[i]] != 0 && !PartOfFigure(newX[i], newX[i]))){
                return;
              }
       }

       for(int i = 0; i < 4; i++){
           X[i] = newX[i];
           Y[i] = newY[i];
       }
       short tmpBoardX = boardX, tmpBoardY = boardY;
       boardX = pivotX + pivotY - tmpBoardY;
       boardY = tmpBoardX + pivotX - pivotY;
        //dodao rotaciju za gornji lijevi ugao
    //metoda sada vrsi i provjeru ispravnosti pozicije na koje bi se trebala figura postaviti nakon rotiranja
    //ako nije ispravno za sad ne radi nista, neki podaci na internetu kazu da se pomijera desno lijevo da bi uspjela rotacija??
   }

    void DrawFigure() {
        for(int i = 0; i < 4; i++) {
            //display je deklarisani display logy iz biblioteke one 
            //računamo gornji lijevi pixel po x i y
            //donji desni dobijemo kad dodamo DIMENZIJU
            //stavio sam 16 za početak, možemo se opet skontati na labu
            //ovo pretpostavlja da nema margina, mogu se lagano dodati uz neku konstantu kao offset
            int upperLeftX = (boardX + X[i]) * DIMENSION, upperLeftY = (boardY + Y[i]) * DIMENSION;
            display.fillrect( upperLeftY, upperLeftX, upperLeftY + DIMENSION, upperLeftX + DIMENSION, colors[colorIndex]);
            //ovo boji granice blokova u crno, možemo skloniti ako ti se ne sviđa   
            display.rect( upperLeftY, upperLeftX, upperLeftY + DIMENSION, upperLeftX + DIMENSION, Black);
        }
    }
    
    void DeleteFigure() {
        for(int i = 0; i < 4; i++) {
            //ista logika kao u DrawFigure, samo popunjavamo sve blokove sa bijelim pravougaonicima
            int upperLeftX = (boardX + X[i]) * DIMENSION, upperLeftY = (boardY + Y[i]) * DIMENSION;
            display.fillrect( upperLeftY, upperLeftX, upperLeftY + DIMENSION, upperLeftX + DIMENSION, White);
        }
    }
    
    void OnAttached() {
        //metoda se poziva kad figura prestanje da se krece
        //popunimo matricu indeksom boje
        for(int i = 0; i < 4; i++) {
            board[boardX + X[i]][boardY + Y[i]] = colorIndex;
            //btw board bi mogao biti niz tipa unsigned char, ali to ćemo vidjet kasnije
        }
    }
    
   bool MoveDown(char delta = 1){
       if(!InCollisionDown(delta)){
            DeleteFigure();
            boardX+=delta;
            DrawFigure();
            return true;
       }
       
       return false;
   }

   void MoveLeft(){
       if(!InCollisionLeft()){
           DeleteFigure();
           boardY--;
           DrawFigure();
       }
   }

   void MoveRight(){
       if(!InCollisionRight()){
           DeleteFigure();
           boardY++;
           DrawFigure();
       }
   }
    
   void SoftDrop() {
       //ovo je funkcija za soft drop
       //obrisemo figuru, postavimo novu poziciju dva reda ispod, nacrtamo figuru
       DeleteFigure();
       MoveDown(2);
       DrawFigure();
       //treba još vidjeti koje izmjene u tickeru trebaju
       score += 2 * (level +1); //prema igrici koju smo igrali, dobije se 14 poena kad se uradi hard drop
   }

   
   bool InCollisionDown(char delta = 1){
       int newX, newY; //da bi bilo citljivije

       for(int i = 0; i < 4; i++){
        newX = boardX + X[i] + delta;
        newY = boardY + Y[i];

        if(BottomEdge(newX) ||
           (board[newX][newY] != 0 && !PartOfFigure(X[i] + delta, Y[i]))){
            return true;
        }
        //jedna figura je u koliziji ako
        //pozicija na koju zelimo da pomjerimo bilo koji blok dotakne dno ili lijevu ili desnu ivicu ekrana
        //ili ako je pozicija na koju zelimo da pomjerimo bilo koji od blokova vec zauzeta a da nije dio figure prije pomijeranja
       }

       return false;
   }

   bool InCollisionLeft(){
       int newX, newY; 

       for(int i = 0; i < 4; i++){
        newX = boardX + X[i];
        newY = boardY + Y[i] - 1;

        if(LeftEdge(newY) ||
           (board[newX][newY] != 0 && !PartOfFigure(X[i], Y[i] - 1))){
            return true;
        }
       }

       return false;
   }


   bool InCollisionRight(){
       int newX, newY;

       for(int i = 0; i < 4; i++){
        newX = boardX + X[i];
        newY = boardY + Y[i] + 1;

        if(RightEdge(newY) ||
           (board[newX][newY] != 0 && !PartOfFigure(X[i], Y[i] + 1))){
            return true;
        }
       }

       return false;
   }

   bool PartOfFigure(int x, int y){
       for(int i = 0; i < 4; i++){
        if(X[i] == x && Y[i] == y) return true;
       }

       return false;
   }
};


Tetromino currentTetromino = 7;


void TickerCallback()
{
    //TODO
}

void OnTasterPressed(){
    if(debounceTaster.read_ms() > 200) {
        if(gameStarted){
            //currentTetromino.Rotate();
        }
        else{
            menu.detach();
            gameStarted = true;
            InitializeDisplay(); //pocinje igra, prikazuje se tabla
            game.attach(&TickerCallback, delays[level]); //svakih nekoliko spusta figuru jedan red nize
        }
        debounceTaster.reset();
    }
}

int main() {
    Init();
    ShowLevelMenu();
    menu.attach(&ReadJoystickForLevel, 0.3);
    taster.mode(PullUp); //mora se aktivirati pull up otpornik na tasteru joystick-a
    taster.fall(&OnTasterPressed);
    debounceTaster.reset();
    debounceTaster.start();
    srand(time(NULL));
    while(1) {
        if(gameStarted) {
            currentTetromino.DrawFigure();
            wait(2);
            Tetromino ok;
            ShowScore();
            while(currentTetromino.MoveDown()) {
                wait(1);
            }
            currentTetromino = ok;  
        }
    }
}
