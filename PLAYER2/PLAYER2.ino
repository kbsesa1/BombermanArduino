#include <Adafruit_STMPE610.h>  // touchscreen driver
#include <Adafruit_ILI9341.h> // Scherm driver
#include <ArduinoNunchuk.h>     // Nunchuck libary
#include <irComm2.h>			// IR library
#include <SPI.h>

IR ir = IR(0);//0 is voor 38 KHz zenden en 1 is voor 56KHz zenden

#define SD_CS 4

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

#define MINPRESSURE 10
#define MAXPRESSURE 1000


// definde de kleuren die we gaan gebruiken
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0


//initialize nunchuck
ArduinoNunchuk nunchuk = ArduinoNunchuk(); 
uint8_t         spi_save;

bool rdy_1 = 0;
bool rdy_2 = 1;
boolean life= true;     // life is true zodat de speler de character mag besturen.
boolean boem = false;   // checkt of de geplaatste bomb al is ontploft.
boolean bombdown = false; // checkt of de speler aan een bomb heeft geplaatst.

int x=200;         // de start x waarde van de character.
int y=270;         // de start y waarde van de character.

int bx;           // bx is de x coordinaat van de geplaatste bomb.
int by;           // by is de y coordinaat van de geplaatste bomb.

//flikkeren scherm tegengaan
int zwart = 0;

int brow ;          // de locatie van de bomb in de grid tenopzichte van de y as.
int bcolumn;        // de locatie van de bomb in de grid tenopzichte van de x as.
     
int row =9;         // de start locatie van de character in de grid tenopzichte van de y as.
int column=13;       // de start locatie van de chacacter in de grid tenopzichte van de x as.


unsigned long startMillis;  // houd bij wanneer de bomb word geplaatst.
unsigned long bombtimer;  // houd bij hoelang de bomb op de grond ligt.

unsigned long startedAt;  // Hoeveel miliseconden is er op moment van aanroepen verstreken.
unsigned long howLong;    // Hoelang er word gewacht totdat we verder gaan.

int direction = 4;        // welke direction de joystick naar toe word geduwt. 

//sla op welke opdracht we als laatste gestuurd hebben zodat we daarna de herhaalopdracht kunnen sturen
int lastlinks=10;		
boolean linksdup=0;

int lastrechts=20;
boolean rechtsdup=0;

int lastomhoog=30;
boolean omhoogdup=0;

int lastomlaag=40;
boolean omlaagdup=0;

//PLAYER2
int p2row= 1;
int p2column =1;

int p2x=40;         // de start x waarde van de tweede character.
int p2y=30;         // de start y waarde van de  tweede character.

int p2brow ;          // de locatie van de tweede bomb in de grid tenopzichte van de y as.
int p2bcolumn;        // de locatie van de tweede bomb in de grid tenopzichte van de x as.

int p2bx;           // bx is de x coordinaat van de tweede geplaatste bomb.
int p2by;           // by is de y coordinaat van de tweede geplaatste bomb.


boolean p2boem = false;   // checkt of de geplaatste bomb al is ontploft.
boolean p2bombdown = false; // checkt of de speler aan een bomb heeft geplaatst.

boolean player2 = true;

unsigned long p2startMillis;  // houd bij wanneer de bomb word geplaatst.
unsigned long p2bombtimer;  // houd bij hoelang de bomb op de grond ligt.

unsigned long irRunTime = 0; //houd bij wanneer de ir gelezen moet worden
unsigned long p2CommandTime = 0; //houd bij wanneer de commando's van p2 uitgevoerd moeten worden



   // |rows  >column
int grid[ 11  ][ 15 ] = {
  
  // [11][15] zijn de max bytes maar bij het aanroepen begin je wel bij 0 
  {1, 1,1,1,1,1,1,1,1,1,1,1,1,1, 1},  //0=air
									  //1=wall    
  {1, 0,0,0,0,0,0,0,0,0,0,0,0,0, 1},  //2=crate
  {1, 0,1,0,1,0,1,0,1,0,1,0,1,0, 1},  //3=bomb
  {1, 2,0,2,0,2,0,2,0,2,0,2,0,0, 1},  //4=explosion
  {1, 0,1,0,1,0,1,0,1,0,1,0,1,0, 1},  //6=p2bomb
  {1, 2,0,2,0,2,0,2,0,2,0,2,0,0, 1},  //7=p2explosie
  {1, 0,1,0,1,0,1,0,1,0,1,0,1,0, 1},
  {1, 2,0,2,0,2,0,2,0,2,0,2,0,2, 1},
  {1, 0,1,0,1,0,1,0,1,0,1,0,1,0, 1},
  {1, 0,0,2,0,2,0,2,0,2,0,2,0,0, 1},
    
  {1, 1,1,1,1,1,1,1,1,1,1,1,1,1, 1}                 
};


enum scherm{Home, Start, Join, Map, Winner, Death };  // benoem de verschillende schermen
typedef enum scherm scherm_t;
scherm_t scherm;

void setup(void) {
	ir.begin();
	ir.sendCommand(0x00,0x04);
  tft.begin();                    // initialiseerd het scherm.
    
  tft.setRotation(1);                 // roteert het scherm 45 graden
  
  //power for nunchuck
  DDRC |= (1<<PORTC2);                // set pin A2 as output
  DDRC |= (1<<PORTC3);                // set pin A3 as output
  PORTC |= (1<<PORTC3);               // set pin A3 high
  wait(100);                      // wait for signal to stabalize
  nunchuk.init();                   // init the nunschuck
  
  if (!ts.begin()) {                  // initialiseerd het touch screen
    while (1);
  }
  scherm = Home;                    // De game is opgestart en het home-screen verschijnt


}

void text_button(int x,int y, int z, String text){  // De functie om een button te tekenen
  tft.setCursor(x, y);              
  tft.setTextColor(BLACK);            // Zet de kleur van tekst op zwart 
  tft.setTextSize(z);
  tft.println(text);
}

void links(){
nunchuk.update();
 if (grid[row][column-1]==0 || grid[row][column-1]== 4) // Checkt of op de postie links van de character niks of een bom explosie zit. 
 {    
 tft.fillRect(y-20,x, 20, 20, GREEN);         // Tekent de character 20 pixels verder naar links.

 if (grid[row][column]!=3)                // Als er op de positie waar de character vandaan komt geen bomb ligt, teken dan dat vlak zwart.
 {
 tft.fillRect(y,x, 20, 20, BLACK);
 }                            
  y=y-20;                       // Zet de coordinaten van de character 20 naar links.
  column=column-1;                  // Past de postie van de character aan op de grid.
  }
    direction=4;

}

void rechts(){
  nunchuk.update();
 
  if (grid[row][column+1]==0 || grid[row][column+1]==4) // Checkt of op de postie rechts van de character niks of een bom explosie zit. 
  {
   tft.fillRect(y+20,x, 20, 20, GREEN);       // Tekent de character 20 pixels verder naar links.
  
  if (grid[row][column]!=3)             // Als er op de positie waar de character vandaan komt geen bomb ligt, teken dan dat vlak zwart.
  {
  tft.fillRect(y,x, 20, 20, BLACK);
  }

  y=y+20;                       // Zet de coordinaten van de character 20 naar links.
  column=column+1;                  // Past de postie van de character aan op de grid.
  }
  direction=4;
}

void omhoog(){
nunchuk.update();
if (grid[row-1][column]==0 || grid[row-1][column]==4 ){ // Checkt of op de postie boven de character niks of een bom explosie zit.
  
  tft.fillRect(y,x-20, 20, 20, GREEN);        // Tekent de character 20 pixels omhoog.
  
  if (grid[row][column]!=3)             // Als er op de positie waar de character vandaan komt geen bomb ligt, teken dan dat vlak zwart.
  {
  tft.fillRect(y,x, 20, 20, BLACK);
  }
  
  x=x-20;                       // Zet de coordinaten van de character 20 omhoog.
  row=row-1;                      // Past de postie van de character aan op de grid.
  }
    direction=4;

}

void omlaag(){
  nunchuk.update();
  if (grid[row+1][column]==0 || grid[row+1][column]==4 )  // Checkt of op de postie onder de character niks of een bom explosie zit.
  {
   
    tft.fillRect(y,x+20, 20, 20, GREEN);          // Tekent de character 20 pixels omlaag.
     
   if (grid[row][column]!=3)                // Als er op de positie waar de character vandaan komt geen bomb ligt, teken dan dat vlak zwart.
   {
    tft.fillRect(y,x, 20, 20, BLACK);
   }

  x=x+20;                         // Zet de coordinaten van de character 20 omlaag.
  row=row+1;                        // Past de postie van de character aan op de grid.
}
  direction=4;

}

void bomb(){

  grid[brow][bcolumn]=4;

  if (grid[brow-1][bcolumn]!=1)
  {
      tft.fillRect(by,bx-20, 20, 20, YELLOW);
      grid[brow-1][bcolumn]=4;

  }

  
  if (grid[brow][(bcolumn+1)]!=1)
  {
      tft.fillRect(by+20,bx, 20, 20, YELLOW);
      grid[brow][bcolumn+1]=4;

  }
  
  if (grid[(brow+1)][bcolumn]!=1)
  {
      
      tft.fillRect(by,bx+20, 20, 20, YELLOW);
      grid[brow+1][bcolumn]=4;
    
  }
    
  if (grid[brow][(bcolumn-1)]!=1)
  {
      
      tft.fillRect(by-20,bx, 20, 20, YELLOW);
      grid[brow][bcolumn-1]=4;
    } 
    boem=false;
    bombtimer = millis();
}
    

void bombtime(){
  if ((bombtimer + 1500)  <= millis() && boem==false)
  {
    for (int i = 0; i < 11; i++ ) {
      for (int j = 0; j < 15; j++ ) {
        if(grid[i][j] ==3 || grid[i][j] == 4 ){
          grid[i][j]=0;
          tft.fillRect((j*20)+10,(i*20)+20, 20, 20, BLACK);
        }
      }
    }
    bombdown=false;
  }
}

void map(){
  tft.fillScreen(BLACK);
  tft.fillRect(y,x, 20, 20, GREEN);
  tft.fillRect(p2y,p2x,20 ,20, BLUE);
  /* Het tekenen van de crate's */
  for (int i = 0; i < 11; i++ ) {
    for (int j = 0; j < 15; j++ ) {
      if(grid[i][j] ==1){
        tft.fillRect((j*20)+10,(i*20)+20, 20, 20, RED);     //alles waar in de grid een 1 staat zijn muren. de muren worden getekent als een rood vierkant
      }
      if(grid[i][j] ==2){
        tft.fillRect((j*20)+10,(i*20)+20, 20, 20, CYAN);    //alles waar in de grid een 2 staat zijn crates. de crates worden getekent als een cyaan vierkant
      }
    }
  }
  

  while(life==true){                          // zolang de character levend is kan die worden bestuurt door speler
    wait(100);
    nunchuk.update();
    //Witte echte controllers.
    //Leest de input van de controller en zet het om in de direction die de character op gaat.  
    if(nunchuk.analogY > 180 && nunchuk.analogX >100 && nunchuk.analogX <180 ) direction = 0; 
    else if(nunchuk.analogY < 65 && nunchuk.analogX >100 && nunchuk.analogX <180 ) direction = 2;
    else if(nunchuk.analogX >  180 && nunchuk.analogY >100 && nunchuk.analogY <180 ) direction = 1;
    else if(nunchuk.analogX < 65 && nunchuk.analogY >100 && nunchuk.analogY <180 ) direction = 3 ;

    if(direction == 0 )
    {
	    if (omhoogdup)		//controleer of we dit commando niet net al hebben gestuurd en stuur anders de herhaalcode
	    {
		    ir.sendCommand(35,0);
		    ir.run();
		    omhoog();
		    omhoogdup = 0;
	    }
	    else{
		    ir.sendCommand(30,0);
		    ir.run();
		    omhoog();
		    omhoogdup = 1;
	    }
    }
    
    if(direction == 2 )
    {
	    if (omlaagdup)
	    {
		    ir.sendCommand(45,0);
		    ir.run();
		    omlaag();
		    omlaagdup = 0;
	    }
	    else{
		    ir.sendCommand(40,0);
		    ir.run();
		    omlaag();
		    omlaagdup = 1;
	    }
    }
    
    if(direction == 1 ) {
	    if (rechtsdup)
	    {
		    ir.sendCommand(25,0);
		    ir.run();
		    rechts();
		    rechtsdup = 0;
	    }
	    else{
		    ir.sendCommand(20,0);
		    ir.run();
		    rechts();
		    rechtsdup = 1;
	    }
    }
    if(direction == 3 ){
	    if (linksdup)
	    {
		    ir.sendCommand(15,0);
		    ir.run();
		    links();
		    linksdup = 0;
	    }
	    else{
		    ir.sendCommand(10,0);
		    ir.run();
		    links();
		    linksdup = 1;
	    }	    
    }
        
    if(nunchuk.zButton==1 && bombdown==false && player2==false){  // Checkt of de z knop is ingedrukt en dat er nog geen bomb op de grond ligt.
	  ir.sendCommand(50,0);
	  ir.run();
	  grid[row][column]=3;          // Zet op de positie van de character in de grid een bomb 
      tft.fillRect(y,x, 20, 20, YELLOW);    // Teken de bomb
      startMillis = millis();         // Onthou wanneer de bomb is geplaatst
      boem=true;                // De bomb is down
      brow=row;               // Sla de coordinaten van de bomb op in nieuwe varriabelen
      bcolumn=column;             // Sla de coordinaten van de bomb op in nieuwe varriabelen 
      bx=x;                 
      by=y;
      bombdown=true;              // bombdown betekent dat er niet nog een bomb down mag
    }
    
    if ((startMillis + 2000  <= millis())  && boem==true )
    {
      bomb();
    }
    
    if (grid[row][column]==4 || grid[row][column]== 7)                   //Als de character in de grid op 4(explosie) staat, gaat de character dood 
    {                   
      scherm = Death;
	  life=false;                         //Life word false

    }
   
	
		if ((bombtimer + 1500)  <= millis() && boem==false)
		{
			for (int i = 0; i < 11; i++ ) {
				for (int j = 0; j < 15; j++ ) {
					if(grid[i][j] ==3 || grid[i][j] == 4 ){
						grid[i][j]=0;
						tft.fillRect((j*20)+10,(i*20)+20, 20, 20, BLACK);
					}
				}
			}
			bombdown=false;
		}

	
	 //////////////////////////////////////////////////////////////////////////
	 //player2
	 //////////////////////////////////////////////////////////////////////////
	
	 if ((p2bombtimer + 1500)  <= millis() && p2boem==false)
	 {
		 for (int i = 0; i < 11; i++ ) {
			 for (int j = 0; j < 15; j++ ) {
				 if(grid[i][j] ==6 || grid[i][j] == 7 ){
					 grid[i][j]=0;
					 tft.fillRect((j*20)+10,(i*20)+20, 20, 20, BLACK);
				 }
			 }
		 }
		 p2bombdown=false;
	 }
	
	  if (grid[p2row][p2column]==4 || grid[p2row][p2column]==7 )                   //Als de character in de grid op 4(explosie) of 7(p2explosie) staat, gaat de character dood
	  {
		  scherm = Winner;
		  life=false;                         //Life word false
	  }
	  
	  if ((p2startMillis + 2000  <= millis())  && p2boem==true )
	  {
		  p2bomb();
	  }
	  
	}
}
  
  

//Eigen delay functie
void wait (unsigned long howLong)
{
  unsigned long startedAt = millis();
  while(millis() - startedAt < howLong)
  {
	  
	  ir.read();
	 switch (ir.getCommand())
	 {
		case 10:		//normale code van naar links
		if(lastlinks==10){
			p2links();

		}
		lastlinks=15;
		break;
		
		case 15:		//herhaal code van naar links
		if(lastlinks==15){
			p2links();
		}
		lastlinks=10;
		break;
		
		case 20:
		if(lastrechts==20){
			p2rechts();

		}
		lastrechts=25;
		break;
		
		case 25:
		if(lastrechts==25){
			p2rechts();
		}
		lastrechts=20;

		break;
		
		case 30:
		if(lastomhoog==30){
			p2omhoog();
		}
		lastomhoog=35;
		break;
		
		case 35:
		if(lastomhoog==35){
			p2omhoog();
		}
		lastomhoog=30;
		break;
		
		
		case 40:
		if(lastomlaag==40){
			p2omlaag();
		}
		lastomlaag=45;
		break;
		
		case 45:
		if(lastomlaag==45){
			p2omlaag();
		}
		lastomlaag=40;
		break;
		
		case 50:
		p2bombneer();
		break;
	}
	
}
}



void loop(){

  // See if there's any  touch data for us
  if (ts.bufferEmpty()) {
    return;
  }
  
  
  TS_Point p = ts.getPoint();
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {

    // Scale from ~0->4000 to tft.width using the calibration #'s
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  }
  
  
  switch(scherm) {
    case Home :                             //home scherm
      if (zwart == 0) {
        tft.fillScreen(BLACK);
        zwart = 1;
      }
      tft.fillRect(70, 98, 90, 40, YELLOW);             //*Start Lobby knop 
      text_button(85, 115, 1, "Start Lobby");

      tft.fillRect(165, 98, 85, 40, YELLOW);              //*Join  Lobby knop
      text_button(178, 115, 1, "Join Lobby");
      
      if (p.x >= 140 && p.x <= 185 && p.y >= 50  && p.y <= 110 ) {
 //       Serial.println("  NEXT STATE = Start");
        scherm = Start;
        zwart = 0;
      } 
      if (p.x >= 140 && p.x <= 185 && p.y >= 120 && p.y <= 180 ) {
   //     Serial.println("  NEXT STATE = Join");
        scherm = Join;
        zwart = 0;
      } 
      break;
      case Start :                          //scherm tekenen bij *Start lobby
      if (zwart == 0) {
      tft.fillScreen(BLACK);  
      zwart = 1;
      }
      tft.fillRect(12, 85, 76, 94, YELLOW);             //*Go knop
      text_button(30, 120, 3, "GO!"); 
      
      tft.fillRect(12, 182, 76, 32, YELLOW);              //*Back knop
      text_button(40, 195, 1, "Back");
                        
      if (p.x >= 90 && p.x <= 200 && p.y >= 15  && p.y <= 60 ) {
        zwart = 0;                          //Vinkje tekenen bij *Go
        int rdy_1 = 1;
        int rdy_2 = 1;
        if(rdy_1 == 1 && rdy_2 == 1) {
          rdy_1 = 0;
          rdy_2 = 0;
          scherm = Map;                     //Naar de game als allebei de spelers RDY zijn
        }

      }
      if (p.x >= 45 && p.x <= 70 && p.y >= 15 && p.y <= 60 ) {    //Home tekenen bij *Back
        zwart = 0;
        rdy_1 = 0;
        rdy_2 = 0;
        scherm = Home;
      }
      break; 
        case Join :                         //scherm tekenen bij *Join lobby
      if (zwart == 0) {
        tft.fillScreen(BLACK);
        zwart = 1;
      }
      tft.fillRect(12, 85, 76, 94, YELLOW);             //*Go knop
      text_button(30, 120, 3, "GO!"); 
      
      tft.fillRect(12, 182, 76, 32, YELLOW);              //*Back knop
      text_button(40, 195, 1, "Back");
      
      if (p.x >= 90 && p.x <= 200 && p.y >= 15  && p.y <= 60 ) {
        zwart = 0;
        int rdy_1 = 1;                
        int rdy_2 = 1;                
        if(rdy_1 == 1 && rdy_2 == 1) {
          rdy_1 = 0;
          rdy_2 = 0;
          scherm = Map;                     //Naar de game als allebei de spelers RDY zijn
        }
      }
      if (p.x >= 45 && p.x <= 70 && p.y >= 15 && p.y <= 60 ) {
        zwart = 0;
        scherm = Home;
        
        rdy_1 = 0;
        rdy_2 = 1;
        scherm = Home;
      }
      break;
        case Map:
        map();                          //voer de functie map uit waar de map word getekent
        
      break;
      
      case Winner:                              //Het win scherm 
        
        tft.fillScreen(MAGENTA);
        tft.fillRect(12, 182, 76, 32, YELLOW);                  //*Back knop
        text_button(40, 195, 1, "Back");
		text_button(90, 80, 4, "WINNER");

        if (p.x >= 45 && p.x <= 70 && p.y >= 15 && p.y <= 60 ) {        //Home tekenen bij *Back
          zwart = 0;
          rdy_1 = 0;
          rdy_2 = 0;
          scherm = Home;
        }
      break;
      break;
        
      case Death:                                 //Scherm bij verliezen
        tft.fillScreen(RED);
        tft.fillRect(12, 182, 76, 32, YELLOW);                  //*Back knop
        text_button(40, 195, 1, "Back");
		text_button(60, 80, 4, "YOU LOSE");

        if (p.x >= 45 && p.x <= 70 && p.y >= 15 && p.y <= 60 ) {        //Home tekenen bij *Back
          zwart = 0;
          rdy_1 = 0;
          rdy_2 = 0;
          scherm = Home;
        }
      break;
    } 
}


//////////////////////////////////////////////////////////////////////////
//PLAYER2
//////////////////////////////////////////////////////////////////////////
void p2links(){
	//if(ontvanged links van player 2){
	if (grid[p2row][p2column-1]==0 || grid[p2row][p2column-1]== 4 || grid[p2row][p2column-1]== 7) // Checkt of op de postie links van de character niks of een bom explosie zit.
	{
		tft.fillRect(p2y-20,p2x, 20, 20, BLUE);         // Tekent de character 20 pixels verder naar links.

		if (grid[p2row][p2column]!=3)                // Als er op de positie waar de character vandaan komt geen bomb ligt, teken dan dat vlak zwart.
		{
			tft.fillRect(p2y,p2x, 20, 20, BLACK);
		}
		p2y=p2y-20;							// Zet de coordinaten van de character 20 naar links.
		p2column=p2column-1;                  // Past de postie van de character aan op de grid.
	}
	
	//}
}

void p2rechts(){
	if (grid[p2row][p2column+1]==0 || grid[p2row][p2column+1]==4 || grid[p2row][p2column+1]==7) // Checkt of op de postie rechts van de character niks of een bom explosie zit.
	{
		tft.fillRect(p2y+20,p2x, 20, 20, BLUE);       // Tekent de character 20 pixels verder naar links.
		
		if (grid[p2row][p2column]!=3)             // Als er op de positie waar de character vandaan komt geen bomb ligt, teken dan dat vlak zwart.
		{
			tft.fillRect(p2y,p2x, 20, 20, BLACK);
		}

		p2y=p2y+20;                       // Zet de coordinaten van de character 20 naar links.
		p2column=p2column+1;                  // Past de postie van de character aan op de grid.
	}
}

void p2omhoog(){
	
	if (grid[p2row-1][p2column]==0 || grid[p2row-1][p2column]==4 || grid[p2row-1][p2column]==7 ){ // Checkt of op de postie boven de character niks of een bom explosie zit.
		
		tft.fillRect(p2y,p2x-20, 20, 20, BLUE);        // Tekent de character 20 pixels omhoog.
		
		if (grid[p2row][p2column]!=3)             // Als er op de positie waar de character vandaan komt geen bomb ligt, teken dan dat vlak zwart.
		{
			tft.fillRect(p2y,p2x, 20, 20, BLACK);
		}
		
		p2x=p2x-20;                       // Zet de coordinaten van de character 20 omhoog.
		p2row=p2row-1;                      // Past de postie van de character aan op de grid.
	}
}

void p2omlaag(){
	if (grid[p2row+1][p2column]==0 || grid[p2row+1][p2column]==4 || grid[p2row+1][p2column]==7 )  // Checkt of op de postie onder de character niks of een bom explosie zit.
	{
		tft.fillRect(p2y,p2x+20, 20, 20, BLUE);          // Tekent de character 20 pixels omlaag.
		
		if (grid[p2row][p2column]!=3)                // Als er op de positie waar de character vandaan komt geen bomb ligt, teken dan dat vlak zwart.
		{
			tft.fillRect(p2y,p2x, 20, 20, BLACK);
		}

		p2x=p2x+20;                         // Zet de coordinaten van de character 20 omlaag.
		p2row=p2row+1;                        // Past de postie van de character aan op de grid.
	}
}


//ontvanged bomb
void p2bomb(){
	
	
	grid[p2brow][p2bcolumn]=6;

	if (grid[p2brow-1][p2bcolumn]!=1)
	{
		//Serial.print("fillRect");
		tft.fillRect(p2by,p2bx-20, 20, 20, YELLOW);
		grid[p2brow-1][p2bcolumn]=7;
	}

	
	if (grid[p2brow][(p2bcolumn+1)]!=1)
	{
		tft.fillRect(p2by+20,p2bx, 20, 20, YELLOW);
		grid[p2brow][p2bcolumn+1]=7;
	}
	
	if (grid[(p2brow+1)][p2bcolumn]!=1)
	{
		tft.fillRect(p2by,p2bx+20, 20, 20, YELLOW);
		grid[p2brow+1][p2bcolumn]=7;
	}
	
	if (grid[p2brow][(p2bcolumn-1)]!=1)
	{
		tft.fillRect(p2by-20,p2bx, 20, 20, YELLOW);
		grid[p2brow][p2bcolumn-1]=7;
	}
	
	p2boem=false;
	p2bombtimer = millis();
}

void p2bombneer(){
	if(p2bombdown==false){				// Checkt of de z knop is ingedrukt en dat er nog geen bomb op de grond ligt.
		grid[p2row][p2column]=6;        // Zet op de positie van de character in de grid een bomb
		tft.fillRect(p2y,p2x, 20, 20, YELLOW);    // Teken de bomb
		p2startMillis = millis();       // Onthou wanneer de bomb is geplaatst
		p2boem=true;					// De bomb is down
		p2brow=p2row;						// Sla de coordinaten van de bomb op in nieuwe varriabelen
		p2bcolumn=p2column;				// Sla de coordinaten van de bomb op in nieuwe varriabelen
		p2bx=p2x;
		p2by=p2y;
		p2bombdown=true;				// bombdown betekent dat er niet nog een bomb down mag
	}
}
