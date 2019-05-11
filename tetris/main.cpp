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
//ticker koji spusta figuru jedan red nize
Ticker t;

const int delay = 1; //svakih 1s se spusti jedan red, ovo provjeriti da li je presporo ili prebrzo
char leftBoundary = 1, rightBoundary = 5; // sada je ovo tipa char
int score = 0;
bool firstTime = true; //ako je prvi put, figura se crta u Tickeru

void ShowScore(void);

void InitializeDisplay()
{
    //ovdje inicijalizujemo display
    display.claim(stdout);
    display.set_orientation(2); // 2 ili 0, zavisi kako okrenemo display, provjerit ćemo na labu kako nam je najlakše povezat
    display.background(White); //bijela pozadina
    display.foreground(Black); //crni tekst
    display.set_font((unsigned char*) Arial12x12);
    display.fillRect(0, 160, 320, 240, Black); //dio za prikazivanje rezultata će biti crni pravougaonik, a tabla je bijeli
    display.fillRect(20, 165, 50, 235, White); //dio za rezultat je bijeli Pravougaonik isto kao tabla
    ShowScore();
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
    return y == 0;
}

bool RightEdge(int y){
    return y == 9;
}

class Tetromino{
private:
    short X[4];
    short Y[4];
    short boardX, boardY;
    unsigned char colorIndex;//dodao sam colorIndex zasad, jer nema drugog načina da popunimo matricu sa indeksima boja
    //ovo je najbezbolnija varijanta što se memorije tiče
    int color;
public:
    Tetromino(){
        srand(time(NULL));
        unsigned char r = rand() % 7 + 1;
        Initialize(r);
    }

    Tetromino(unsigned char colorIndex) {
        Initialize(colorIndex);
    }

    void Initialize(unsigned char colorIndex) {
        this.color = colors[colorIndex];
        this.colorIndex = colorIndex;
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
    
    void OnAttached() {
        //metoda se poziva kad figura prestanje da se krece
        //popunimo matricu indeksom boje
        for(int i = 0; i < 4; i++) {
            board[boardX + X[i]][boardY + Y[i]] = colorIndex;
            //btw board bi mogao biti niz tipa unsigned char, ali to ćemo vidjet kasnije
        }
    }
    
   bool MoveDown(){
       if(!InCollisionDown()){
            DeleteFigure();
            boardX++;
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

   bool InCollisionDown(){
       int newX, newY; //da bi bilo citljivije

       for(int i = 0; i < 4; i++){
        newX = boardX + X[i] + 1;
        newY = boardY + Y[i];

        if(BottomEdge(newX) ||
           (board[newX][newY] != 0 && !PartOfFigure(X[i] + 1, Y[i]))){
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


Tetromino currentTetromino();

void CheckLines(short &firstLine, short &numberOfLines)
{
    //vraća preko reference prvu liniju koja je popunjena do kraja, kao i takvih linija
    firstLine = -1; //postavljen na -1 jer ako nema linija koje treba brisati ispod u DeleteLines neće se ući u petlju
    numberOfLines = 0;
    for(int i = 19; i >= 0; i--) {
        short temp = 0;
        for(int j = 0; j < 10; j++) {
            if(board[i][j] == 0) break; //ako je makar jedna bijela, prekida se brojanje
            temp++;
        }
        if(temp == 10) { //ako je temo došao do 10, niti jedna bijela - puna linija
            numberOfLines++;
            if(firstLine == -1) firstLine = i; //ovo mijenjamo samo prvi put
        }
    }
}


int UpdateScore (short numOfLines){ 
    int newIncrement = 0;
    switch(numOfLines) {
        case 1 : newIncrement = 40; break;
        case 2 : newIncrement = 100; break;
        case 3 : newIncrement = 300; break;
        case 4 : newIncrement = 1200; break;
        default : newIncrement = 0; break;
    }
    return newIncrement;
}

void UpdateBoard()
{
    short firstLine, numberOfLines;
    CheckLines(firstLine, numberOfLines); //pozivamo funkciju
    for(int i = firstLine; i >= numberOfLines; i--) {
        for(int j = 0; j < 10; j++) {
            board[i][j] = board[i - numberOfLines][j];
            display.fillrect(i * DIMENSION, j * DIMENSION, i * (DIMENSION + 1), j * (DIMENSION + 1), colors[board[i][j]]); // bojimo novi blok
            if(board[i][j] != 0) 
                display.rect(i * DIMENSION, j * DIMENSION, i * (DIMENSION + 1), j * (DIMENSION + 1), Black);
            //ako nije bijela boja, crtamo granice
        }
    }
    for(int i = 0; i < firstLine; i++) {
        for(int j = 0; j < 10 ; j++) {
            board[i][j] = 0; //u prvih onoliko redova koliko su obrisani stavljamo 0
            display.fillrect(i * DIMENSION, j * DIMENSION, i * (DIMENSION + 1), j * (DIMENSION + 1), White); 
            //bojimo nove blokove u bijelo
        }
    }
    score += UpdateScore(numberOfLines); //ovdje se mijenja globalna varijabla score
}


void ShowScore() {
    //pomocna funkcija za prikazivanje score-a
    display.fillRect(20, 165, 50, 235, White); //popunimo pravugaonik da obrišemo stari score
    display.locate(35, 200); //valjda je na sredini pravougaonika
    printf("%d", score);
}

void ReadJoystick() {
    if(VRx < leftBoundary / 6.0) {
        leftBoundary = 2;
        currentTetromino.MoveLeft();
    }
    else if(VRx > rightBoundary / 6.0) {
        rightBoundary = 4;
        currentTetromino.MoveRight();
    }
    else {
        leftBoundary = 1;
        rightBoundary = 5;
    }
}

bool IsOver() {
    for(int i = 0; i < 10; i++) {
        if(board[0][j] != 0) return true;
    }
    return false;
}

void ShowGameOverScreen() {
    display.cls();
    display.background(Black);
    display.foreground(White);
    display.locate(120, 50);
    printf("GAME OVER");
    display.locate(150, 30);
    printf("YOUR SCORE IS %d", score);
}

void TickerCallback(){
    if(firstTime) {
        currentTetromino.DrawFigure();
        firstTime = false;
    }
    
    ReadJoystick();
    
    if(!currentTetromino.MoveDown())){
        currentTetromino.OnAttached();
        UpdateBoard();
        ShowScore();
        
        srand(time(NULL));
        unsigned char nextColor = rand() % 7 + 1;  
        currentTetromino = Tetromino(nextColor);
        currentTetromino.DrawFigure();
        
        if(IsOver()) {
            //ako je igra završena brišemo sve sa displey-a, prikazujemo poruku i score
            //takođe moramo dettach-at ticker
            t.dettach();
            ShowGameOverScreen();
            score = 0;
            firstTime = true;
        }
    }
}

int main()
{
    rotateBtn.mode(PullUp); //mora se aktivirati pull up otpornik na tasteru joystick-a
    rotateBtn.rise(&currentTetromino, &Tetromino::Rotate); //na uzlaznu ivicu
    t.attach(&TickerCallback, delay); //spusta jedan red nize svake sekunde
    
    InitializeDisplay(); //ovdje se uključi display
    while(1) {
        //vidjet ćemo ide li išta u while
    }
    return 0;
}
