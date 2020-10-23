#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <x86intrin.h>

const int BUFFER_LEN = 100010;
const uint64_t INF = 0x7FF0000000000000;
const uint64_t NaN = 0x7FF00000001F1E33;
const uint64_t NINF = 0xFFF0000000000000;

uint64_t read_from_string(char*);
char* write_to_string(uint64_t);

uint64_t add(uint64_t, uint64_t);
uint64_t subtract(uint64_t, uint64_t);
uint64_t multiply(uint64_t, uint64_t);
uint64_t divide(uint64_t, uint64_t);

inline int Prior(char);
inline int Sign(uint64_t);
inline bool isNaN(uint64_t);
inline bool isINF(uint64_t);
inline uint64_t Exp(uint64_t);
inline uint64_t LowBit(uint64_t);
inline uint64_t Fraction(uint64_t);
inline uint64_t Negative(uint64_t);
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
		bool negop = false;
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
				if(negop){
					*(numtop - 1) = Negative(*(numtop - 1));
					negop = false;
				}
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
			}else if(*cur == '-' && noprev){
				negop = true;
				cur++;
			}
			else{
				while(opstop != stackops && Prior(*(opstop - 1)) >= Prior(*cur)){
					uint64_t rhs = *(--numtop);
					uint64_t lhs = *(--numtop);
					*(numtop++) = Evaluate(lhs, rhs, *(--opstop));
				}
				*(opstop++) = *cur;
				if(*cur == '(')
					noprev = true;
				cur++;
			}
		}
		for(char* x = cur; *x != '\0'; x++)
			putchar(*x);
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
	return (Exp(x) == (1 << 11) - 1) && (Fraction(x) & ((1ull << 52) - 1)) == 0;
}

uint64_t add(uint64_t lhs, uint64_t rhs){
//	return d2u(u2d(lhs) + u2d(rhs));

	if(isNaN(lhs) || isNaN(rhs))
		return NaN;

	if(Sign(lhs) != Sign(rhs)) // only handle same sign
		return subtract(lhs, Negative(rhs));

	if(isINF(lhs) || isINF(rhs)){
		if(isINF(lhs))
			return lhs;
		else{
			assert(isINF(rhs));
			return rhs;
		}
	}

	if(Exp(lhs) < Exp(rhs)){
		uint64_t tmp = lhs;
		lhs = rhs;
		rhs = tmp;
	}
	
	int ediff = Exp(lhs) - Exp(rhs);

	uint64_t ans = 0;
	bool extra = false;
	uint64_t ansexp = Exp(lhs);
	uint64_t rhsf = Fraction(rhs) << 2;
	uint64_t ansf = Fraction(lhs) << 2;

	while(rhsf != 0){
		uint64_t cur = LowBit(rhsf) >> ediff;
		if(cur == 0)
			extra = true;
		else
			ansf += cur;
		rhsf -= LowBit(rhsf);
	}

	// Adjust EXP
	while(ansf >= (1ull << 55)){
		++ansexp;
		extra = extra || (ansf & 1) != 0;
		ansf >>= 1;
	}
	// Rounding
	if((ansf & 3) < 2)
		ansf >>= 2;
	else if((ansf & 3) > 2)
		ansf = (ansf >> 2) + 1;
	else{
		assert((ansf & 3) ==2);
		if(extra)
			ansf = (ansf >> 2) + 1;
		else{ // ties to even
			ansf >>= 2;
			if((ansf & 1) != 0)
				++ansf;
		}
	}
	// NOTE: only 011111 -> 100000, no more rounding required
	if(ansf >= (1ull << 53)){
		++ansexp;
		ansf >>= 1;
	}

	if(ansexp >= (1ull << 11)) // overflow
		ans = INF;
	else
		ans = ansexp << 52 | (ansf & ((1ull << 52) - 1));

	ans |= (1ull << 63) & lhs; // Add sign
	return ans;
}

uint64_t subtract(uint64_t lhs, uint64_t rhs){
//	return d2u(u2d(lhs) - u2d(rhs));

	if(isNaN(lhs) || isNaN(rhs))
		return NaN;

	if(Sign(lhs) != Sign(rhs)) // only handle same sign
		return add(lhs, Negative(rhs));

	if(isINF(lhs) && isINF(rhs))
		return NaN;
	if(isINF(lhs) || isINF(rhs))
		return INF;
	if(lhs == rhs)
		return 0; // avoid unexpected negative 0

	bool negflag = false;
	if(Exp(lhs) < Exp(rhs) || (Exp(lhs) == Exp(rhs) &&Fraction(lhs) < Fraction(rhs))){
		negflag = true;
		uint64_t tmp = lhs;
		lhs = rhs;
		rhs = tmp;
	}

	int ediff = Exp(lhs) - Exp(rhs);

	uint64_t ans = 0;
	bool extra = false;
	uint64_t ansexp = Exp(lhs);
	uint64_t ansf = Fraction(lhs) << 3;
	uint64_t rhsf = Fraction(rhs) << 3;

	while(rhsf != 0){
		uint64_t cur = LowBit(rhsf) >> ediff;
		if(cur == 0)
			extra = true;
		else
			ansf -= cur;
		rhsf -= LowBit(rhsf);
	}

	// Adjust EXP
	while(ansexp > 0 && (ansf & (1ull << 55)) == 0){
		--ansexp;
		ansf <<= 1;
	}
	// Rounding
	if((ansf & 7) < 4)
		ansf >>= 3;
	else if((ansf & 7) > 4)
		ansf = (ansf >> 3) + 1;
	else{
		if(extra)
			ansf >>= 3;
		else{
			ansf >>= 3;
			if((ansf & 1) != 0)
				++ansf;
		}
	}
	// NOTE: only 011111 -> 100000, no more rounding required
	if(ansf >= (1ull << 53)){
		++ansexp;
		ansf >>= 1;
	}

	ans = ansexp << 52 | (ansf & ((1ull << 52) - 1));

	ans |= lhs & (1ull << 63); // Add sign
	return negflag ? Negative(ans) : ans;
}

uint64_t multiply(uint64_t lhs, uint64_t rhs){
	return d2u(u2d(lhs) * u2d(rhs));
}

uint64_t divide(uint64_t lhs, uint64_t rhs){
	return d2u(u2d(lhs) / u2d(rhs));
}

inline uint64_t Evaluate(uint64_t lhs, uint64_t rhs, char op){
	uint64_t ans;
	switch(op){
		case '+':
			ans = add(lhs, rhs);
			break;
		case '-':
			ans = subtract(lhs, rhs);
			break;
		case '*':
			ans = multiply(lhs, rhs);
			break;
		case '/':
			ans = divide(lhs, rhs);
			break;
		default:
			assert(false);
	}
#ifdef PALL
	printf("%f %c %f = %f\n",u2d(lhs),op,u2d(rhs),u2d(ans));
#endif
	return ans;
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
	uint64_t x;
	sscanf(str, "%lf", &x);
	return x;
}

char* write_to_string(uint64_t x){
	char* ans = malloc(BUFFER_LEN * sizeof(char));
	if(isNaN(x))
		strcpy(ans, "nan");
	else if(isINF(x))
		strcpy(ans, Sign(x) > 0 ? "inf" : "-inf");
	else
		sprintf(ans, "%.1200f", _mm_cvtsi64_si128(x));
	return ans;
}

inline uint64_t LowBit(uint64_t x){
	return x & ((~x) + 1);
}

inline uint64_t Negative(uint64_t x){
	return x ^ (1ull << 63);
}

inline uint64_t Exp(uint64_t x){
	return (x >> 52) & ((1 << 11) - 1);
}

inline int Sign(uint64_t x){
	return (x >> 63) == 0 ? 1 : -1;
}

inline uint64_t Fraction(uint64_t x){
	return (x & ((1ull << 52) - 1)) | (Exp(x) ? 1ull << 52 : 0);
}
