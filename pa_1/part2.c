// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 1

#include <stdio.h>
#include <stdlib.h>

typedef unsigned int u32;

/*======================= Part 2 ========================================*/

//Given: putchar(char c) of Linux, which prints a char.
//2-1. Write YOUR own prints(char *s) fucntion to print a string.
int prints(char *s){
    while(*s){
        putchar(*s);
        s++;
    }
}

// Given: The following printu() function prints an unsigned integer.
char *ctable = "0123456789ABCDEF";
int  BASE = 10, BASEhex = 16, BASEoct = 8; 

int rpu(u32 x)
{  
    char c;
    if (x){
       c = ctable[x % BASE];
       rpu(x / BASE);
       putchar(c);
    }
}

int printu(u32 x)
{
   (x==0)? putchar('0') : rpu(x);
   //putchar(' ');
}

// 2-2. Write YOUR ONW functions
// int  printd(int x) which prints an integer (x may be negative!!!)
int printd(int x){
    if (x < 0){ 
        putchar('-');
        x *= -1;
    }
    printu(x);
}

// rpu for hex
int hexRpu(u32 x){
    char c;
    if(x){
        c = ctable[x % BASEhex];
        hexRpu(x / BASEhex);
        putchar(c);
    }
}

// int  printx(u32 x) which prints x in HEX   (start with 0x )
int printx(u32 x){
    putchar('0');
    putchar('x');
    (x==0)? putchar('0') : hexRpu(x);
    putchar(' ');

}

// rpu for oct
int OctRpu(u32 x){
    char c;
    if(x){
        c = ctable[x % BASEoct];
        OctRpu(x / BASEoct);
        putchar(c);
    }
}

// int  printo(u32 x) which prints x in Octal (start with 0  )
int printo(u32 x){
    putchar('0');
    (x==0)? putchar('0') : OctRpu(x);
    putchar(' ');
}


/* 3. REQUIREMENTS:
====================================================================
  Write YOUR own myprintf(char *fmt, ...) function to print 
         char                      by %c
         string                    by %s
         unsigned integer          by %u
         integer                   by %d
         unsigned integer in OCT   by %o
         unsigned integer in HEX   by %x
   Ignore field width and precision, just print the items as specified. */

int myprintf(char *fmt, ...){
    char *cp = fmt;
    // input variable is one address higher than fmt
    int *ip = (int *)&fmt + 1;

    while(*cp){
        // print the string until we get %
        if (*cp != '%'){
            putchar(*cp);
            // when get n check if it is \n
            // if true insert \r
            if (*cp == '\n'){
                putchar('\r');
                /*cp++;
                if (*cp == 'n'){
                    putchar('\\');
                    putchar('r');
                }*/
            }
        }
        // when % is found, get next char
        // and call appropiate print function
        else{
            cp++;
            switch (*cp){
                case 'c':
                    putchar(*ip);
                    ip++;
                    break;
                case 's':
                    prints(*ip);
                    ip++;
                    break;
                case 'u':
                    printu(*ip);
                    ip++;
                    break;
                case 'd':
                    printd(*ip);
                    ip++;
                    break;
                case 'o':
                    printo(*ip);
                    ip++;
                    break;
                case 'x':
                    printx(*ip);
                    ip++;
                    break;
                // case '%%' should print %
                case '%':
                    putchar('%');
                    break;
                // default case print nothing
                // have weird behavior in normal printf()
                default:
                    break;
            }
        }
    // go to next char
    cp++;
    }
}

int myprintfTest(){
    char a = 'a';
    char *s = "you";
    u32 ui = 420;
    int itg = 240;
    myprintf("\nTest myprintf: \n");
    myprintf("char test: a = %c\n", a);
    myprintf("string test: you = %s\n", s);
    myprintf("unsigned int: 420 = %u\n", ui);
    myprintf("int: 240 = %d\n", itg);
    myprintf("Oct UInt: 420 = 644 = %o\n", ui);
    myprintf("Hex Uint: 420 = 1A4 = %x\n", ui);
}

/* 2-2. In the int main(int argc, char *argv[ ], char *env[ ]) function, 
     use YOUR myprintf() to print
              argc value
              argv strings
              env  strings
 
    myprintf("cha=%c string=%s      dec=%d hex=%x oct=%o neg=%d\n", 
	       'A', "this is a test", 100,    100,   100,  -100);*/

int main(int argc, char *argv[ ], char *env[ ]){
    int i;
    myprintfTest();

    // argc
    //printf("argc = %d\n", argc);
    myprintf("argc = %d\n", argc);

    // argv[]
    for(i = 0; i < argc; i++){
        myprintf("argv[%d] = %s\n", i, argv[i]);
    }

    // env[]
    i = 0;
    while(env[i]){
        myprintf("env[%d] = %s\n", i, env[i]);
        i++;
    }

    // final test
    myprintf("cha=%c string=%s      dec=%d hex=%x oct=%o neg=%d\n", 
	    'A', "this is a test", 100,    100,   100,  -100);

    return 0;
}