#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/ioport.h>

#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#include <linux/version.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

#include <linux/uaccess.h>
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ALU driver");

#define DRIVER_NAME "alu"

//pomocne funkcije
static unsigned long strToInt (const char* pStr, int len, int base);
static char chToUpper(char ch);
static int intToStr(int val, char* pBuf, int bufLen, int base);

//globalne promenljive
int endRead =0 ;
static dev_t first;
static struct class *cl;
static struct cdev c_dev;

struct alu_info {
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};

static struct alu_ingo *lp = NULL;

//FILE operations (fops)

static int alu_open (struct inode *pinode, struct file *pfile) {
	printk(KERN_INFO "Succ opened file\n");
	return 0;
}

static int alu_close (struct inode *pinode, struct file *pfile){
	printk(KERN_INFO "Succ closed file\n");
	return 0;
}

static ssize_t alu_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset){
	u32 res;
	int len;
	char buf[10] = {0};

	if(endRead) {
		printk(KERN_INFO "Succ read from file\n");
		end read=0;
		return 0;
	}
	res = ioread32(lp->base_addr+12);
	
	printk(KERN_INFO "Rezultat je 0x%X.\n", res);
	
	len = intToStr(res, buf, 10,10);
	
	if(copy_to_user(buffer,buf,len) ) {
		return -EFAULT;
	}
	
	endRead=1;
	
	return len;
}

static ssize_t alu_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{

	char buff[100]; //prom kojoj cu dodeliti sve sto sam primio sa bafera
	char operand1[48]={0}, operand2[48]={0}, Operacija[4] = {0};
	//prom za dodeljivianje operandi u vidu stringa, kao i operacije
	
	char op1_0x[46]={0}, op2_0x[46] = {0}; 	//prom za citanje hex unesenih operandi
											//razlika je u 2 mesta koja ce hex operande prvo zauzeti (0x)
	
	int ret; 	//ocitavanje unosa
	int i;		//za for petlju
	int count=0;
	/*pom prom koja igra ulogu brojaca, evidentira mi kada je dodeljen poslednji
	* karakter nekom poslednjem stringu, a zatim se sa tom novom vrednoscu ga koristim
	* za punjenje drugog stringa, preko raznih ifova u for petlji */
	
	int tz1,tz2; 
	/*flag koji mi govori na kom je i-tom clanu stringa buff detektovan prvi i drugi
	* delimetar - tacka/zarez (;) */
	
	int dozvola_operand22=1; 
	/*kada imam dva operanda onda ova dozvola pusta oba operanda da se obradjuju
	* i salju dalje ka ALU, medjutim kada imam jedan dodelim mu vrednost 0
	* i preko ifa njega ignorisem. Ovo je napravljeno zbog operacija koje koriste samo
	* jedan operand, kao sto su shr ili ror */
	
	unsigned long int prom_Op1 =0, pom_Op2=0;
	/*promenljiva koja string operand1 i operand2 pretvara u integer
	* nije finalna promenljiva jer jos treba proveriti da li je u pitanju 32bitni broj
	* ukoliko je postao negativan onda znamo da je presao 32 bita, a ukoliko nije 
	* dodelujemo ga dalje sledecim promenljivama */
	
	u32 Operand1=0, Operadn2=0; //ove promenljive operandi se upisuju u ALU registre
	u32 op=0; 					//ova promenljiva je operacija koja se upisuje u ALU registar
	//krece se u opsegu od 0 do 7 za razlicite operacije koje odradjuje nasa ALU
	
	int check=0, flag_enter =0; //jos neke pomocne promenljive
	
	ret = copy_from_user(buff, buffer, 100);
	
	///////////////////////////////////////
	// PARSIRANJE STRINGA /////////////////
	///////////////////////////////////////
	
	for(i=0;i<100;i++){
		if(buff[i] != ';' && count == 0) {	
			operand1[i] = buff[i];
		}
		if(buff[i] == ';' && count == 0) {	
			tz1 = i;
			count++;
		} //sve do prvog delimetra je operand1
		
		/*ovo se odnosi na drugi operand*/
		if(buff[i] != ';' && count == 1) {	
			operand2[i-tz1-1] = buff[i]; 
			//pocinje od nultog clana operanda2 pa se oduzima sa tz1,1
		}
		/*u isto vreme dok punimo u operand2 punicemo i u string za operaciju
		* za slucaj da koristimo jedan operand i operaiciju */
		if(buff[i] != ';' && buff[i] != 10 && count == 1 && i != (tz1+4) ) {
		/*ovaj uslov mi kaze da mi puni u op string za slucaj da nije dosao do ;
		* ili za slucaj da je dosao do kraja unesenog stringa, kraj stringa ima vrednost 10
		* a stavili smo i da je i mora biti biti razlicito od tz1+4 jer jer op max duzine 4*/
			Operacija[i-tz1-1] = buff[i];
		}
		if(buff[i] == 10 && i == (tz1+4) && count==1) {
			/*reaguje na cetvrtom mestu jer svaka operacija koja radi sa jednim
			* operandom ima 3 slova (shr,shl,ror,rol), a 4 slovo je ono koje ne treba
			* da unosi */
			count = 3;			//preskoci mi sledece ifove za dodelu operandu2
								//kao i operaciji ponovo
			dozvola_operand22=0; //nemamo drugi operand;
		}
		//ponovo za operand2
		if(buff[i] == ';' && count == 1 && i != tz1) {
			tz2 = i;
			count++;
			//ako nije evidentiran kraj stringa u proslom ifu
			//onda ce mi ovde evidentirati kraj operanda2 u jednom trenutku
			//a zatim ide da puni ponovo operaciju
			//odnosno da upisuje preko vec upisanih char clanova stringa operacija
		}//kraj operanda2
		
		if(buff[i] != 10 && count == 2) {
			Operacija[i-tz2-1] = buff[i];
		}
		if(buff[i] == 10 && count == 2 && i!=tz2) {
			count++ //count = 3;
		}
		
		/*pomocni if za evidentiranje entera*/
		if(buff[i] == 10 && flag_entera == 0) {
			flag_entera = i; //flag za dalji rad
		}
		
	}//kraj for petlje
	
	///////////////////////////////////////
	// PRETVARANJE OPERANDA ///////////////
	// U BROJEVE , BILO HEX BILO DEC //////
	///////////////////////////////////////
	// kao i provera da li ima pogr unosa//
	///////////////////////////////////////
	
	if(operand1[1] != 'x') {
		/*ako mi je prvi clan razlicit od x to znaci 
		* da sigurno nije hexadec zapis u pitanju */
		
		for(i=0;i<tz1;i++){
			if((operand1[i] != '0' && operand1[i] != '1' && operand1[i] != '2' && operand1[i] != '3' &&
				operand1[i] != '4' && operand1[i] != '5' && operand1[i] != '6' && operand1[i] != '7' &&
				operand1[i] != '8' && operand1[i] != '9') && check ==0 ) {
			
				//ako neki od clanova operanda nije broj imamo gresku pri unosu!
				check++;
				goto failop1;
			}//if
		}//for
		
		if(check == 0) 
			pom_Op1 = strToInt(operand1, tz1, 10);
	}
	else if(operand1[0]!='0' && operand1[1] == 'x' ) {
		goto failop1;
	}
	else if(operand1[0]=='0' && operand1[1] == 'x' ){
		/*ukoliko imamo 0 i x onda imamo hexadec unos, ali pre toga provera da li
		* se unose pravilni karakteri u koje spadaju samo sledeci karakteri:
		*
		* 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F */
		for(i=2;i<tz1;i++) {
			if((operand1[i] != '0' && operand1[i] != '1' && operand1[i] != '2' && operand1[i] != '3' &&
				operand1[i] != '4' && operand1[i] != '5' && operand1[i] != '6' && operand1[i] != '7' &&
				operand1[i] != '8' && operand1[i] != '9' && operand1[i] != 'a' && operand1[i] != 'A' &&
				operand1[i] != 'b' && operand1[i] != 'B' && operand1[i] != 'c' && operand1[i] != 'C' &&
				operand1[i] != 'd' && operand1[i] != 'D' && operand1[i] != 'e' && operand1[i] != 'E' &&
				operand1[i] != 'f' && operand1[i] != 'F') && check ==0 ) {
				
				check++;
				goto failop1;
			}//if
		}//for
		if(check == 0) {
			for(i=2;i<tz1;i++ ) {
				op1_0x[i-2] = operand1[i];
			}
			pom_Op1 = strToInt(op1_0x, (tz1-2) , 16);
		}//if
	}
	else goto failop1;
	
	//operand2
	if(dozvola_operand22 == 1) {
		if(operand2[1] != 'x') {
			/*ako mi je prvi clan razlicit od x to znaci 
			* da sigurno nije hexadec zapis u pitanju */
			
			for(i=0;i<tz1;i++){
				if((operand2[i] != '0' && operand2[i] != '1' && operand2[i] != '2' && operand2[i] != '3' &&
					operand2[i] != '4' && operand2[i] != '5' && operand2[i] != '6' && operand2[i] != '7' &&
					operand2[i] != '8' && operand2[i] != '9') && check ==0 ) {
				
					//ako neki od clanova operanda nije broj imamo gresku pri unosu!
					check++;
					goto failop2;
				}//if
			}//for
			
			if(check == 0) 
				pom_Op2 = strToInt(operand2, tz1, 10);
		}
		else if(operand2[0]!='0' && operand2[1] == 'x' ) {
			goto failop2;
		}
		else if(operand2[0]=='0' && operand2[1] == 'x' ){
			/*ukoliko imamo 0 i x onda imamo hexadec unos, ali pre toga provera da li
			* se unose pravilni karakteri u koje spadaju samo sledeci karakteri:
			*
			* 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F */
			for(i=2;i<(tz2-tz1-1);i++) {
				if((operand2[i] != '0' && operand2[i] != '1' && operand2[i] != '2' && operand2[i] != '3' &&
					operand2[i] != '4' && operand2[i] != '5' && operand2[i] != '6' && operand2[i] != '7' &&
					operand2[i] != '8' && operand2[i] != '9' && operand2[i] != 'a' && operand2[i] != 'A' &&
					operand2[i] != 'b' && operand2[i] != 'B' && operand2[i] != 'c' && operand2[i] != 'C' &&
					operand2[i] != 'd' && operand2[i] != 'D' && operand2[i] != 'e' && operand2[i] != 'E' &&
					operand2[i] != 'f' && operand2[i] != 'F')&& check ==0 ) {
					
					check++;
					goto failop2;
				}//if
			}//for
			if(check == 0) {
				for(i=2;i<tz1;i++ ) {
					op2_0x[i-2] = operand2[i];
				}
				pom_Op2 = strToInt(op2_0x, (tz2-tz1-1-2) , 16);
			}//if		
		}
		else goto failop2;
	}//kraj if(dozvola_operand22)
	
	
	/////////////////////////////////////
	//provera da li operandi imaju vise//
	///////////od 32 bita ///////////////
	/////////////////////////////////////
	
	if(pom_Op1 < 0 || pom_Op1 > 2147483647) {
		goto fail2;
	}
	else Operand1 = pom_Op1; //u32
	
	if(pom_Op2 < 0 || pom_Op2 > 2147483647) {
		goto fail2;
	}
	else Operand2 = pom_Op2; //u32
	
	//dodela vrednosti registrima A i B
	iowrite32(Operand1, lp->base_addr);
	iowrite32(Operadn2, lp->base_addr+4);
	
	//////////////////////////////////////////////////////
	//razne provere vezane za operaciju
	//
	//da li je dozvoljena duzina unosa operanda
	//(svaka operacija ima 3 slova osim or)
	//
	//da li je uneseno tacno to sto treba da se unese/////
	//
	//da li se unosi pod dozvoljenim uslovima
	//(neke operacije se pisu za dva, a neke za
	//jedan operand) //////////////////////////////////////
	///////////////////////////////////////////////////////
	
	if(((flag_enter-tz2-1) > 3 && dozvola_operand22 == 1) ||
	   ((flag_enter-tz1-1) > 3 && dozvola_operand22 == 0) ) {
	   //ako imamo vise od 3 slova onda je greska
	   goto fail4;
	}
	
	if( Operacija[0] == 'a' && Operacija[1] == 'n' && Operacija[2] == 'd') {
		if(dozvola_operand22) op = 0; //and se upisuje u registar za operaciju kao nula
		else goto fail6;
	}
	else if( Operacija[0] == 'o' && Operacija[0] == 'r') {
		if(dozvola_operand22) {
			if( (flag_enter-tz2-1)==2) op = 1; //or u reg kao 1 
											   //duzina strina operacije mora biti 2
			else goto fail4;
		}
		else goto fail6;
	}
	else if( Operacija[0] == 'a' && Operacija[1] == 'd' && Operacija[2] == 'd') {
		if(dozvola_operand22) op = 2;
		else goto fail6;
	}
	else if( Operacija[0] == 's' && Operacija[1] == 'u' && Operacija[2] == 'b') {
		if(dozvola_operand22) op = 3;
		else goto fail6;
	}
	else if( Operacija[0] == 's' && Operacija[1] == 'h' && Operacija[2] == 'l') {
		if(!dozvola_operand22) op = 4;
		else goto fail5;
	}
	else if( Operacija[0] == 's' && Operacija[1] == 'h' && Operacija[2] == 'r') {
		if(!dozvola_operand22) op = 5;
		else goto fail5;
	}
	else if( Operacija[0] == 'r' && Operacija[1] == 'o' && Operacija[2] == 'l') {
		if(!dozvola_operand22) op = 6;
		else goto fail5;
	}
	else if( Operacija[0] == 'r' && Operacija[1] == 'o' && Operacija[2] == 'r') {
		if(!dozvola_operand22) op = 7;
		else goto fail5;
	}
	else op=10;
	
	if (op>=0 && op<=7) {
		//dodela op_sel registru
		iowrite32(op, lp->base_addr+8);
	}
	else goto fail3;
	
	printk(KERN_INFO "Succ written into file\n");
	return length;
	
failop1: {
		printk(KERN_INFO "Uneli ste nedozvoljene karaktere u okviru operanda1!\n");
		goto fail;
	}
	
failop2: {
		printk(KERN_INFO "Uneli ste nedozvoljene karaktere u okviru operanda2!\n");
		goto fail;
	}
	
fail2: {
		printk(KERN_INFO "Uneseni operand ima vise od 32 bita!\n");
		goto fail;
	}
	
fail3: {
		printk(KERN_INFO "Uneli ste nepostojecu operaciju!\n");
		goto fail;
	}
	
fail4: {
		printk(KERN_INFO "Uneli ste vise karaktera nego sto je dozvoljeno zao operaciju!\n");
		goto fail;
	}
	
fail5: {
		printk(KERN_INFO "Ova operacija se vrsi prilikom unosa samo jednog operanda!\n");
		goto fail;
	}
	
fail6: {
		printk(KERN_INFO "Ova operacija se vrsi prilikom unosa oba operanda!\n");
		goto fail;
	}
	
fail:{
		printk(KERN_ALERT "Unsucc written info file\n"); //red alert
		return -1;
	}	
	
}//kraj funkcije write

/////POMOCNE FUNKCIJE///////////////////////////

static unsigned long strToInt (const char* pStr, int len, int base){
	static const int v[] = {0,1,2,4,5,6,7,8,9,0,0,0,0,0,0,0,10,11,12,13,14,15};
	unsigned long val = 0;
	int i, dec =1, idx=0;
	
	for(i=len;i>0;i--){
		idx = chToUpper(pStr[i-1]) - '0';
		
		if(idx > sizeof(v)/sizeof(int) ) {
			printk("strToInt: illegal character %c\n", pStr[i-1] );
			continue;
		}
		
		val += (v[idx])*dec;
		dec *= base;
	}
	return val;
}
static char chToUpper(char ch){
	if( (ch >= ''A && ch <= 'Z') || (ch >= '0' && ch <= '9') ) 
		return ch;
	else
		return (ch - ('a'-'A') );
}
static int intToStr(int val, char* pBuf, int bufLen, int base){
	static const char *pConv = "0123456789ABCDEF";
	int num = val, len = 0, pos = 0;
	
	while(num>0) {
		len++;
		num /= base;
	}
	if (val == 0) len = 1;
	
	pos = len-1;
	num = val;
	
	if(pos > bufLen-1) pos = bufLen-1;
	
	for(;pos>=0;pos--) {
		pBuf[pos] = pConv[num % base];
		num /= base;
	}
	
	return len;
}

/////////////////////////////////////////

static int alu_probe(struct platform_device *pdev) {
	//funkc a povezivanje virtuelne i fizicke memorije
	//mapiranje registara
	
	struct resource *r_mem;
	int rc =0;
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0) ;
	if(!r_mem) {
		printk(KERN_ALERT "invalid address\n");
		return -ENODEV;
	}
	
	lp= (struct alu_info *) kmalloc(sizeof(struct alu_info), GFP_KERNEL) ;
	if(!lp) {
		printk(KERN_ALERT "couldnt allocate alu dev\n");
		return -ENOMEM;
	}
	
	lp->mem_start = r_mem->start;
	lp->mem_end = r_mem->end;
	
	//request mem region for alu driver, based on resources read from device tree
	if(!request_mem_region(lp->mem_start, lp->mem_end - lp->mem_start +1, DRIVER_NAME) )
	{
		printk(KERN_ALERT "couldnt loc mem region at %p\n", (void *)lp->mem_start);
		rc=-EBUSY;
		goto error1;
	}
	
	//remap to virt memory which will be used to access alu from user space application
	lp->base_addr = ioremap(lp->mem_start, lp->mem_end - lp->mem_start+1);
	if(!lp->base_addr) {
		printk(KERN_ALERT "ALU: couldnt allocate iomem\n");
		rc = -EIO;
		goto error2;
	}
	
	return 0;

error2:
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	
error1:
	return rc;
}

static int alu_remove(struct platform_device *pdev) {
	printk(KERN_ALERT "ALU platf driver removed\n");
	iowrite32(0,lp->base_addr);
	iounmap(lp->base_addr);
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	//release also mem allocated for alu_info struct
	kfree(lp);
	
	return 0;
}	
	
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

static struct of_device_id alu_of_match[] = {
	{.compatible = "xlnx, ALU-unit-1.0", }, //povezovamke sa hardverom
	{ /* end of list */ },
};

MODULE_DEVICE_TABLE(of, alu_of_match);

static struct file_operations my_fops = {
	//dodeljivanje funkcije strukturi
	//struktura ciji su param funkcije
	.owner = THIS_MODULE,
	.open = alu_open,
	.release = alu_close,
	.read = alu_read,
	.write = alu_write
};

static struct platform_driver alu_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = alu_of_match,
		},
		.probe = alu_probe,
		.remove = alu_remove,
};

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

static int __init alu_init(void) {
	printk(KERN_INFO "ALU init.\n");
	
	if(alloc_chrdev_region(&first, 0,1, "ALU region") <0) {
		printk(KERN_ALERT "<1>Failed CHRDEV!\n");
		return -1;
	}
	printk(KERN_INFO "Succ CHRDEV!\n");
	
	
	if((cl = class_create(THIS_MODULE, "chardrv")) == NULL){
		printk(KERN_ALERT "<1>Failed Class create!\n");
		goto fail_0;
	}
	printk(KERN_INFO "Succ class chardev1 create.!\n");


	if(device_create(cl,NULL, MKDEV(MAJOR(first),0), NULL, "ALU" ) == NULL)
	{
		goto fail_1;
	}
	printk(KERN_INFO "Device created.\n");
	
	cdev_init(&c_dev, &my_fops);
	if(cdev_add(&c_dev, first, 1) == -1) {
		goto fail_2;
	}
	printk(KERN_INFO "Device init.\n");
	
	printk(KERN_INFO "\n\nHello worldl\n\n");
	
	return platform_driver_register(&alu_driver);
	
fail_2:
	device_destroy(cl, MKDEV(MAJOR(first),0) );

fail_1:
	class_destroy(c1);
	
fail_0:
	unregister_chrdev_region(first,1);
	return -1;
}

static void __exit alu_exit(void) {
	
	device_destroy(c1, MKDEV(MAJOR(first),0) );
	class_destroy(cl);
	unregister_chrdev_region(first,1);
	cdev_del(&c_dev);
	platform_driver_unregister(&alu_driver);
	
	printk(KERN_ALERT "ALU exit.\n");
	printk(KERN_INFO "\n\nGoodbye, cruel world\n\n");
}

module_init(alu_init);
module_exit(alu_exit);
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	