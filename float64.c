#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <x86intrin.h>

const int BUFFER_LEN=100010;

uint64_t read_from_string(char*);
char* write_to_string(uint64_t);
uint64_t add(uint64_t, uint64_t);
uint64_t subtract(uint64_t, uint64_t);
uint64_t multiply(uint64_t, uint64_t);
uint64_t divide(uint64_t, uint64_t);

inline int Prior(char);
inline int Exp(uint64_t);
inline int Sign(uint64_t);
inline bool isNaN(uint64_t);
inline bool isINF(uint64_t);
inline uint64_t Fraction(uint64_t);
inline uint64_t Evaluate(uint64_t, uint64_t, char);

#ifdef IVE
inline uint64_t d2u(double x){
	return *((uint64_t*)((void*)&x));
}
inline double u2d(uint64_t x){
	return *((double*)((void*)&x));
}
#endif

typedef __int128 intEx;

int main(){ // Function: Parse & Evaluate
	char* const expr = malloc(BUFFER_LEN * sizeof(char));
	char* const stackops = malloc(BUFFER_LEN * sizeof(char));
	uint64_t* const stacknum = malloc(BUFFER_LEN * sizeof(uint64_t));

	char* opstop = stackops;
	uint64_t* numtop = stacknum;

	gets(expr);

	{
		bool noprev = true;
		char* cur = expr;
		while(*cur != '\0'){
			if(isspace(*cur))
				cur++;
			else if(isdigit(*cur)){
				noprev=false;
				char* num = malloc(BUFFER_LEN * sizeof(char));
				char* numcur = num;
				while(*cur != '\0' && (isdigit(*cur) || *cur == '.'))
					*(numcur++) = *(cur++);
				*(numcur++) = '\0';
				*(numtop++) = read_from_string(num);
				free(num);
			}
			else if(*cur == ')'){
				while(*(opstop - 1) != '('){
					assert(opstop != stackops);
					uint64_t rhs = *(--numtop);
					uint64_t lhs = *(--numtop);
					*(numtop++) = Evaluate(lhs, rhs, *(--opstop));
				}
				--opstop;
				cur++;
			}
			else if(*cur == '('){
				*(opstop++) = *cur;
				cur++;
				noprev = true;
			}
			else{
//				printf("$ %c\n",*cur);
				while(opstop != stackops && Prior(*(opstop - 1)) >= Prior(*cur)){
					uint64_t rhs = *(--numtop);
					uint64_t lhs = *(--numtop);
					*(numtop++) = Evaluate(lhs, rhs, *(--opstop));
				}
				if(*cur == '-' && noprev)
					*(numtop++) = 0;
				*(opstop++) = *cur;
				if(*cur == '(')
					noprev = true;
				cur++;
			}
		}
		while(opstop != stackops){
			uint64_t rhs = *(--numtop);
			uint64_t lhs = *(--numtop);
			*(numtop++) = Evaluate(lhs, rhs, *(--opstop));
		}
	}

	char* ans = write_to_string(*stacknum);
	printf("%s\n", ans);
	free(ans);

	free(expr);
	free(stacknum);
	return 0;
}

inline bool isNaN(uint64_t x){
	return (Exp(x) == (1 << 11) - 1) && (Fraction(x) & ((1ull << 52) - 1)) != 0;
}

inline bool isINF(uint64_t x){
	return (Exp(x) == (1 << 11) - 1) && (Fraction(x) & ((1ull << 52) - 1))  == 0;
}

uint64_t add(uint64_t lhs, uint64_t rhs){
	return d2u(u2d(lhs) + u2d(rhs));
}

uint64_t subtract(uint64_t lhs, uint64_t rhs){
	return d2u(u2d(lhs) - u2d(rhs));
}

uint64_t multiply(uint64_t lhs, uint64_t rhs){
	return d2u(u2d(lhs) * u2d(rhs));
}

uint64_t divide(uint64_t lhs, uint64_t rhs){
	return d2u(u2d(lhs) / u2d(rhs));
}

inline uint64_t Evaluate(uint64_t lhs, uint64_t rhs, char op){
//	printf("%f %c %f\n", u2d(lhs), op, u2d(rhs));
	switch(op){
		case '+':
			return add(lhs, rhs);
		case '-':
			return subtract(lhs, rhs);
		case '*':
			return multiply(lhs, rhs);
		case '/':
			return divide(lhs, rhs);
		default:
			assert(false);
	}
}

inline int Prior(char x){
	switch(x){
		case '+':
		case '-':
			return 0;
		case '*':
		case '/':
			return 1;
		case '(':
			return -1;
		default:
			printf("ERROR: '%c'\n", x);
			assert(false);
	}
}

uint64_t read_from_string(char* str){
//	printf("read \"%s\", ",str);
	uint64_t x;
	sscanf(str, "%lf", &x);
//	printf("result = %f\n", u2d(x));
	return x;
}

char* write_to_string(uint64_t x){
	char* ans = malloc(BUFFER_LEN * sizeof(char));
//	printf("\"0x%016lx\"\n",x);
	if(isNaN(x))
		strcpy(ans, "nan");
	else if(isINF(x))
		strcpy(ans, Sign(x) > 0 ? "inf" : "-inf");
	else
		sprintf(ans, "%.1200f\n", _mm_cvtsi64_si128(x));
	return ans;
}

inline int Exp(uint64_t x){
	return (x >> 52) & ((1 << 11) - 1);
}

inline int Sign(uint64_t x){
	return (x >> 63) == 0 ? 1 : -1;
}

inline uint64_t Fraction(uint64_t x){
	return (x & ((1ull << 52) - 1)) | (Exp(x) ? 1ull << 52 : 0);
}
