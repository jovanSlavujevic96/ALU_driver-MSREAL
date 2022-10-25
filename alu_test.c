#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define AND 0
#define OR 1
#define ADD 2
#define SUB 3
#define SHL 4
#define SHR 5
#define ROL 6
#define ROR 7
#define MNOZENJE_NULOM 8
#define MNOZENJE_JEDINICOM 9
#define MNOZENJE 10

void error_msg(int i); //greske
void upis_file(char operand1[50], char operand2[50], char operacija[10] ); //upis u ALU
void ispis_file(char rez[100]); //dobijeni rezultat drajvera, kao string
int check_operacija(char operacija[10], char OPERACIJA[4], char OPERAND2[50]); 
	//pretvaranje + u add i slicno
int check_operand2(char operand2[50]);
	//dobijanje operanda2 kao integera bio hex ili dec string
char povecajSlova (char maloSlovo); 
	//funkcija koja mi vraca velika slova A,B,C,D,E,F za njihova mala slova
int hexTodec(char hexVal[]);
	//pretvaranje hexadec chara u dec integer, funkcija za povecanje slova je ukljucenja u njoj
void operacija_unutar_zagrade(char operand_zagrada[50], char novi_operand[50]);
	//funkcija za racunanje unutar zagrade i dodelivanje konacnom operandu
void operacija_nekoliko_puta (char operand1[50], char operand2[50], char operacija[10], char OPERACIJA[4], char REZ[100]);
	//ova funkcija je zaduzena da izvrsi krajnji proracun i poveze sve gore navedene funkcije
	//ukoliko postoji mnozenje, shiftovanje ili rotiranje nekoliko puta
void prepisivanje_stringa(char dobijeni[], char zeljeni[] ); //pomocna funkcija
void prepisivanje_operanda(char operand[] );

FILE *FP;
int GRESKA=0;

int main(int argc, char **argv){

	if(argv[1] == NULL) error_msg(0);
	if(argv[2] != NULL) error_msg(-1);
	
	int i=0, space_check=0, space[2]={0};
	char operand1[48]={0}, operand2[48]={0}, operacija[10]={0}, rez[100]={0}, OPERACIJA[4]={0};
	char rez1[100]={0}, rez2[100];
	
	for(i=0;i<strlen(argv[1])) ; i++) {
		if(argv[1][i] != 32 && space_check == 0) {
			operand1[i] = argv[1][i];
		}
		if(argv[1][i] == 32 && space_check == 0) {
			space_check++;
			space[0]=i;
		}
		if(argv[1][i] != 32 && space_check == 1) {
			operacija[i-space[0]-1] =argv[1][i];
		}
		if(argv[1][i] == 32 && space_check == 1 && i!=space[0]) {
			space_check++;
			space[1]=i;
		}
		if(i > space[1] && i < strlen(argv[1]) && space_check ==2) {
			operand2[i-space[1]-1] = argv[1][i];
		}
	}//for

	operacija_unutar_zagrade(operand1,rez1);
	operacija_unutar_zagrade(operand2,rez2);
	operacija_nekoliko_puta(rez1,rez2,operacija,OPERACIJA,rez);
	
	if(GRESKA == -1 ) return -1;
	
	return 0;
}

void operacija_nekoliko_puta (char operand1[50], char operand2[50], char operacija[10], char OPERACIJA[4], char REZ[100])
{
	int i;
	char oPrnd2[7]={0};
	char NULL[6]= {[0]='N', [1]='U', [2]='L', [3]='L', 0};	
	char NULA[2] = {'0','0'};
	
	int dodela_za_operaciju = check_operacija(operacija, OPERACIJA, operand2);
	
	if(dodela_za_operaciju >= 4 && dodela_za_operaciju <= 10) {
		int koliko_puta = check_operand2(operand2);
		
		if(dodela_za_operaciju >= 4 && dodela_za_operaciju <=7) prepisivanje_stringa(NUL, oPrnd2);
		if(dodela_za_operaciju == MNOZENJE_JEDINICOM) prepisivanje_stringa(NULA, oPrnd2);
		if(dodela_za_operaciju == MNOZENJE || dodela_za_operaciju == MNOZENJE_NULOM) prepisivanje_stringa(operand1, oPrnd2);
		
		if(koliko_puta >1) {
			char rez[1000][100] = {{0}};
			
			upis_file(operand1,oPrdn2, OPERACIJA);
			ispis_file(rez[0]);
			
			if(dodela_za_operaciju == 10) koliko_puta--;
			if(koliko_puta>1) {
				for(i=1;i<koliko_puta;i++) {
					upis_file(rez[i-1],oPrdn2,OPERACIJA);
					ispis_file(rez[i]);
				}
				prepisivanje_stringa(rez[koliko_puta-1],REZ);
			}
		}
		else {
			char rez[100] = {0};
			
			upis_file(operand1, oPrdn2, OPERACIJA);
			ispis_file(rez);
			prepisivanje_stringa(rez,REZ);
		}
	}
	else if( dodela_za_operaciju >= 0 && dodela_za_operaciju < 4) {
		char rez[100] = {0};
		upis_file(operand1, operand2, OPERACIJA);
		ispis_file(rez);
		prepisivanje_stringa(rez,REZ);
	}
}

void proveravanje_operanda(char operand[] ){
	int i,chk=0;
	if(operand[1] != 'x') {
		for(i=0;i<strlen(operand);i++) {	
			if((operand[i] != '0' && operand[i] != '1' && operand[i] != '2' && operand[i] != '3' && operand[i] != '4' &&
			   operand[i] != '5' && operand[i] != '6' && operand[i] != '7' && operand[i] != '8' && operand[i] != '9') && chk==0)
			{
				chk++;
				error_msg(4);
			}
		}
	}
	else if(operand[0] != '0' && operand[1] == 'x') {
		error_msg(4);
	}
	else if(operand[0] == '0' && operand[1] == 'x') {
		for(i=2;i<strlen(operand);i++) {
			if((operand[i] != '0' && operand[i] != '1' && operand[i] != '2' && operand[i] != '3' && operand[i] != '4' &&
			 operand[i] != '5' && operand[i] != '6' && operand[i] != '7' && operand[i] != '8' && operand[i] != '9' &&
			 operand[i] != 'a' && operand[i] != 'A' && operand[i] != 'b' && operand[i] != 'B' && operand[i] != 'c' &&
			 operand[i] != 'C' && operand[i] != 'd' && operand[i] != 'D' && operand[i] != 'e' && operand[i] != 'E' &&
			 operand[i] != 'f' && operand[i] != 'F' ) && chk==0) {
				chk++;
				error_msg(4);
			}
		}
	}			 
}

void prepisivanje_stringa(char dobijeni[], char pretvoreni[] ) {
	int i;
	//pomocna funkcija za prepisivanje stringa
	//moglo se koristiti i cpy
	for (i=0;i<strlen(dobijeni);i++){
		pretvoreni[i] = dobijeni[i];
	}
}

void operacija_unutar_zagrade(char operand_zagrada[50], char novi_operand[50]) {
	int i, chk=0, flag_kraj_op1=0, flag_kraj_oprc=0;
	int a = strlen(operand_zagrada)-1;
	char op1[50]={0}, op2[50]={0},oprc[10]={0}, OPERACIJA[4]={0};
	if(operand_zagrada[0]=='(' && operand_zagrada[a] == ')' ) {
		for(i=1;i<a;i++) { //obidji zagrade
			if(operand_zagrada[i] != '+' && operand_zagrada[i] != '-' && operand_zagrada[i] != '*' && operand_zagrada[i] != '&' &&
			   operand_zagrada[i] != '|' && operand_zagrada[i] != '<' && operand_zagrada[i] != '>' && operand_zagrada[i] != 'r' &&
			   operand_zagrada[i] != 'o' && operand_zagrada[i] != 'l' ) {
				
				if(chk==0) op1[i-1]=operand_zagrada[i];
				if(chk==1 && i > flag_kraj_op1) {
					chk++;
					flag_kraj_oprc=i;
				}
				if(chk==2 && i>=flag_kraj_oprc && i<a) op2[i-flag_kraj_oprc]=operand_zagrada[i];
			}
			else {
				if(chk==0) {
					flag_kraj_op1=i;
					chk++;
				}
				if(i>= flag_kraj_op1 && chk==1) {
					oprc[i-flag_kraj_op1]=operand_zagrada[i];
				}
			}
		}
		proveravanje_operanda(op1);
		proveravanje_operanda(op2);
		operacija_nekoliko_puta(op1,op2,oprc,OPERACIJA,novi_operand);
	}
	else if(operand_zagrada[0]!='(' && operand_zagrada[a] != ')' ){
		//ako nema zagrada onda samo prepisi operand i salji dalje
		for(i=0;i<(a+1);i++){
			novi_operand[i] = operand_zagrada[i]
			proveravanje_operanda(novi_operand);
		}
	}
	else error_msg(3);
	//ukoliko postoji jedna a jedna ne postoji to je greska
}

int hexTodec(char hexVal[] ) {
	//pomocna funkcija za pretvaranje stringa koji se
	//sastoji od hexadec kar se pretvara u int
	
	int len = strlen(hexVal);
	int base = 1;
	int dec_val = 0;
	int i;
	
	for(i=len-1;i>=0;i--){
		if(hexVal[i]>= '0' && hexVal[i] <= '9') {
			dec_val += (hexVal[i] -48)*base;
			base = base*16;
		}
		else if(hexVal[i] >= 'A' && hexVal[i] <= 'F') {
			dec_val += (hexVal[i] -55)*base;
			base = base*16;
		}
	}
	return dec_val;
}

char povecajSlova(char maloSlovo) {
	char tmp=0;
	
	//mala slova abcdef se pretvaraju u velika
	if(maloSlovo == 'a') tmp = 'A';
	else if(maloSlovo == 'b') tmp = 'B';
	else if(maloSlovo == 'c') tmp = 'C';
	else if(maloSlovo == 'd') tmp = 'D';
	else if(maloSlovo == 'e') tmp = 'E';
	else if(maloSlovo == 'f') tmp = 'F';
	else tmp = 0;
	
	return tmp;
}

int check_operand2(char operand2[50]) {
		//pretvaranje operanda2 (stringa) u integer
		int i;
		int chk=0, tmp=0;
		char tmpc[49] = {0};
		
		if(operand2[1] != 'x' ) { //nije hexadec
			//posto nije hexadec onda proveravamo da li u sebi sadrzi samo brojeve 0123456789
			//ako sadrzi jos nesto treba da mi posalje gresku
			for(i=0;i<strlen(operand2);i++){
				if(operand2[i] != '0' && operand2[i] != '1' && operand2[i] != '2' && operand2[i] != '3' &&
				   operand2[i] != '4' && operand2[i] != '5' && operand2[i] != '6' && operand2[i] != '7' &&
				   operand2[i] != '8' && operand2[i] != '9' ) {
					
					if(chk==0) {
						chk++;
						error_msg(1);
					}
				}
				else {
					if(chk==0 && i=(strlen(operand2)-1) ) {
						tmp=atoi(operand2);
						chk++;
					}
				}
			}
			
		}
		if(operand2[0] != '0' && operand2[1] == 'x' ) {
			//varijanta kada mi je prvi clan razlicit od nule a drugi je jednak x
			//nije pravilan unos
			//samo ako unesemo 0x____ onda imamo hexadec
			error_msg(1);
		}
		
		if(operand2[0] == '0' && operand2[1] == 'x') {
			for(i=0;i<strlen(operand2);i++){
				if(operand2[i] != '0' && operand2[i] != '1' && operand2[i] != '2' && operand2[i] != '3' &&
				   operand2[i] != '4' && operand2[i] != '5' && operand2[i] != '6' && operand2[i] != '7' &&
				   operand2[i] != '8' && operand2[i] != '9' &&
				   operand2[i] != 'a' && operand2[i] != 'A' && operand2[i] != 'b' && operand2[i] != 'B' && operand2[i] != 'c' &&
				   operand2[i] != 'C' && operand2[i] != 'd' && operand2[i] != 'D' && operand2[i] != 'e' && operand2[i] != 'E' &&
				   operand2[i] != 'f' && operand2[i] != 'F') 
				{
					if(chk==0) 
						chk++,
						error_msg(1);
				}
				else{
					if(chk==0){
						if(operand2[i] == 'a' || operand2[i] == 'b' || operand2[i] == 'c' || operand2[i] == 'd' || operand2[i] == 'e' ||operand2[i] == 'f' )
						{
							tmpc[i-2]=povecajSlova(operand2[i]);
							//ukoliko su slova mala pretvori ih u velika slova
						}
						else {
							//sve ostale varijante samo puni u pomocni string
							tmpc[i-2]=operand2[i];
						}
					}
				}
				if(chk ==0 && i == (strlen(operand2)-1) ) {
					//kada smo dosli do kraja pretvori mi ovaj tmp string u broj
					tmp = hexTodec(tmpc);
				}
			}
		}
		
		return tmp;
}

int check_operacija(char operacija[10], char OPERACIJA[4], char OPERAND2[50] ){
	if(operacija[0] == '+' && strlen(operacija) == 1) {
		OPERACIJA[0] = 'a' ; OPERACIJA[1] = 'd'; OPERACIJA[2] = 'd';
		return ADD;
	}
	if(operacija[0] == '-' && strlen(operacija) == 1) {
		OPERACIJA[0] = 's' ; OPERACIJA[1] = 'u'; OPERACIJA[2] = 'b';
		return SUB;
	}
	if(operacija[0] == '&' && strlen(operacija) == 1) {
		OPERACIJA[0] = 'a' ; OPERACIJA[1] = 'n'; OPERACIJA[2] = 'd';
		return AND;
	}
	if(operacija[0] == '|' && strlen(operacija) == 1) {
		OPERACIJA[0] = 'o' ; OPERACIJA[1] = 'r'; 		
		return OR;
	}
	if(operacija[0] == '>' && operacija[1] == '>' && strlen(operacija) == 2) {
		OPERACIJA[0] = 's' ; OPERACIJA[1] = 'h'; OPERACIJA[2] = 'r';
		return SHR;
	}
	if(operacija[0] == '<' && operacija[1] == '<' && strlen(operacija) == 2) {
		OPERACIJA[0] = 's' ; OPERACIJA[1] = 'h'; OPERACIJA[2] = 'l';
		return SHL;
	}
	if(operacija[0] == 'r' && operacija[1] == 'o' && operacija[2] == 'r' && strlen(operacija) == 3) {
		OPERACIJA[0] = 'r' ; OPERACIJA[1] = 'o'; OPERACIJA[2] = 'r';
		return ROR;
	}
	if(operacija[0] == 'r' && operacija[1] == 'o' && operacija[2] == 'l' && strlen(operacija) == 3) {
		OPERACIJA[0] = 'r' ; OPERACIJA[1] = 'o'; OPERACIJA[2] = 'l';
		return ROL;
	}
	if(operacija[0] == '*' && strlen(operacija) == 1) {
		//za slucaj da se mnozi trebalo bi da nekoliko puta da se jedan broj sa sobom sabere
		//ukoliko se pomnozi sa jedan treba da se sabere sa nulom
		//a ukoliko se pomnozi sa nulom onda treba da se broj oduzme sam sa sobom
		int operand2 = check_operand2(OPERAND2);
		//ovim proveravamo da sta je vrednost drugog operanda, zbog toga je potrebno da string pre slanja u drajver da pretvorim
		//u broj i vidimo da li se moze OPERAND2 nesto uraditi sa tim, takodje se to odnosi i na shiftovanje i na rotiranje
		if (operand2 == 0){
			OPERACIJA[0] = 's' ; OPERACIJA[1] = 'u'; OPERACIJA[2] = 'b';
			return MNOZENJE_NULOM;
		}
		else if(operand2 == 1){
			OPERACIJA[0] = 'a' ; OPERACIJA[1] = 'd'; OPERACIJA[2] = 'd';
			return MNOZENJE_NULOM;
		}
		else if(operand2 > 1){
			OPERACIJA[0] = 'a' ; OPERACIJA[1] = 'd'; OPERACIJA[2] = 'd';
			return MNOZENJE;
		}
	}
	error_msg(2);
	//ako dodeljena operacija ne izgleda ni na jedan ovih nacina onda imamo gresku
	return (-1);
}

void upis_file(char operand1[50], char operand2[50], char operacija[10] ){
	char tmp[100]={0};
	
	if(GRESKA!=-1){
		strcpy(tmp,operand1);
		tmp[strlen(tmp)] = ';' ;
		
		if(operand2[0] == 'N' && operand2[0] == 'U' && operand2[0] == 'L' && operand2[0] == 'L' && strlen(operand2)==4)
		{
			strcat(tmp,operacija);
			tmp[strlen(tmp)]=10;
		}
		else{
			strcat(tmp,operand2);
			tmp[strlen(tmp)]=';' ;
			strcat(tmp,operacija);
			tmp[strlen(tmp)]=10;
		}
		
		FP = fopen("/dev/ALU","w");
		
		if(FP==NULL) error_msg(-2);
		
		fputs(tmp,FP);
		fclose(FP);
	}
}

void ispis_file(char rez[100] ) {
	int i, chk=0;
	char tmp[100]={0};
	
	if(GRESKA!=-1){
		FP = fopen("/dev/ALU","r");
		
		if(FP==NULL) error_msg(-2);
		
		for(i=0;i<100;i++){
			if( !feof(FP) && chk==0) tmp[i] = fgetc(FP);
			if(  feof(FP) && chk==0) chk++;
		}
		chk=0;
		fclose(FP);
		
		for(i=0;i<100;i++){
			if( tmp[i]!=255 && chk==0) rez[i] = tmp[i];
			if( tmp[i]==255 && chk==0) chk++;
		}
		
	}
}

void error_msg(int i){
	if(i==-2) printf("Greska!\nNismo uspeli da udjemo u fajl\n");
	if(i==-1) printf("Greska!\nUneli ste vise argumenata nego sto treba!\n");
	if(i==0) printf("Greska!\nPogresan unos\nPravilan unos: op1 & op2\n");
	if(i==1) printf("Greska!\nNedozvoljeni karakteri unutar operanda2\n");
	if(i==2) printf("Greska!\nNepravilan unos operacije\n");
	if(i==3) printf("Greska!\nPosedujete jednu zagradu a nemate drugu!\n");
	if(i==4) printf("Greska!\nNepravilan unos nekog od operanda\n");
	
	GRESKA=-1;
}


