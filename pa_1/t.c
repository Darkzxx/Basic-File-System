// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 1

/************* t.c file ********************/
#include <stdio.h>
#include <stdlib.h>

int *FP;

int main(int argc, char *argv[ ], char *env[ ])
{
  int a,b,c;
  int i;
  printf("enter main\n");
  
  printf("&argc=%x argv=%x env=%x\n", &argc, argv, env);
  printf("&a=%8x &b=%8x &c=%8x\n", &a, &b, &c);

//(1). Write C code to print values of argc and argv[] entries

  // argc
  printf("argc = %d\n", argc);

  // argv[]
  for(i = 0; i < argc; i++){
    printf("argv[%d] = %s\n", i, argv[i]);
  }

  a=1; b=2; c=3;
  A(a,b);
  printf("exit main\n");
}

int A(int x, int y)
{
  int d,e,f;
  printf("enter A\n");
  // PRINT ADDRESS OF d, e, f
  printf("&d = %8x  &e = %8x  &f = %8x\n", &d, &e, &f);
  d=4; e=5; f=6;
  B(d,e);
  printf("exit A\n");
}

int B(int x, int y)
{
  int g,h,i;
  printf("enter B\n");
  // PRINT ADDRESS OF g,h,i
  printf("&g = %8x  &h = %8x  &i = %8x\n", &g, &h, &i);
  g=7; h=8; i=9;
  C(g,h);
  printf("exit B\n");
}

int C(int x, int y)
{
  int u, v, w, i, *p;
  int *fp;
  int n;

  printf("enter C\n");
  // PRINT ADDRESS OF u,v,w,i,p;
  // p = &u; -> pointer test
  printf("&u = %8x  &v = %8x  &w = %8x  &i = %8x  p = %8x\n", &u, &v, &w, &i, &p);
  u=10; v=11; w=12; i=13;

  FP = (int *)getebp();

  //(2). Write C code to print the stack frame link list.
  fp = FP;
  printf("Stack Frame link list\n");
  // stack frame at crt0.o = 0
  while(fp != 0){
    printf("FP = %8x\n", fp);
    fp = *fp;
  }
  printf("FP = %8x\n", fp);

  p = (int *)&p;
  //(3). Print the stack contents from p to the frame of main()
  //    YOU MAY JUST PRINT 128 entries of the stack contents.
  fp = p;
  printf("stack content from p to frame of main\n");
  n = 128;
  while(n > 0){
    printf("(%d) -> %8x -> %d\n", n, fp, *fp);
    // p to main -> low to high address
    fp++;
    n--;
  }

  //(4). On a hard copy of the print out, identify the stack contents
  // as LOCAL VARIABLES, PARAMETERS, stack frame pointer of each function.
}