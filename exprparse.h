

#include <stdlib.h>
/*Expression scanner*/
/*Expressions are like c, but with only three valid basetypes and no arrays, just pointers.
no structs or unions.

	Instead of local variable identifiers, special expressions are used...
	@u***(stackaddr)
	will specify that the stack address in question (before the start of the expression, of course) holds
	an unsigned int***.

	For global variables...

	$u***(globaladdr)

	if you had an expression in C

	PtrToMyMemory[i * 3] = x + 5;

	where PtrToMyMemory is unsigned int*, i is int, and x is unsigned int, and all are stack variables,

	with stack arrangement like so:

	stackpointer->______________
					PtrToMyMemory (starting at stp-4)
					i 			  (starting at stp-8)
					x 			  (starting at stp-12)

	then...

	

	$   $u***(4) [$i(8) * 3] = $u(12) + 5;

	would be the equivalent expression.
	notice that the line begins with a dollar sign.

	that indicates that it is an expression line.

	It must end in a semicolon.

	no function calls are allowed in an expression (YET)


	The following operators are supported, 
	with the same operators applicable to unsigned being applicable to signed integers:
	(LEFT-TO_RIGHT)

	PARSE_LEVELIDENT
	identifier 
	($|@)(u|i|f){*}\(scan_unsigned\)

	PARSE_LEVELIDENT_OR_NUMBER
	identifier or number.
	We recognize an identifier because it begins with $ or @.
	PARSE_LEVEL29 || unsigned || signed || float
	
	PARSE_LEVELPAREN
	() (any expression inside)
	(PARSE_LEVEL1) || PARSE_LEVEL28


	PARSE_LEVELBRACK
	[] (requires lvalue pointer)

	(RIGHT-TO-LEFT)

	PARSE_LEVEL21
	unary + (requires rvalue integer)

	PARSE_LEVEL20
	unary - (requires rvalue integer)

	PARSE_LEVEL19
	unary ! (requires rvalue integer)

	PARSE_LEVEL18
	unary ~ (requires rvalue integer)

	PARSE_LEVEL17
	(type) (explicit cast)

	PARSE_LEVEL16
	* indirection (requires rvalue pointer, generates lvalue)

	PARSE_LEVEL15
	unary & (requires lvalue)


	(LEFT-TO_RIGHT)

	PARSE_LEVEL14
	* / % (% cannot be done on a float, all require two rvalues.)

	PARSE_LEVEL13
	+ - (requires two rvalues)

	PARSE_LEVEL12
	<< >> (requires two rvalues, integers)

	PARSE_LEVEL11
	< <= (two rvalues, again)

	PARSE_LEVEL10
	> >= (two rvalues, again)

	PARSE_LEVEL9
	== != (two rvalues, again)

	PARSE_LEVEL8
	& (two rvalues, integers)

	PARSE_LEVEL7
	^ (two rvalues, integers)

	PARSE_LEVEL6
	| (two rvalues, integers)

	PARSE_LEVEL5
	= (lvalue on left, rvalue on right, types must match or be converted.)

	PARSE_LEVEL4
	+= -= (lvalue on left, rvalue on right, types must match or be converted)


	PARSE_LEVEL3
	*= /= %= (same)
	PARSELEVEL4 { ("*=" || "/=" || "%=") PARSE_LEVEL4 }

	PARSE_LEVEL2
	<<= >>= (same, integers only) 
	PARSELEVEL3 { ("<<=" || ">>=") PARSE_LEVEL3}

	PARSE_LEVEL1
	&= ^= |= (same, integers only) 
	PARSE_LEVEL2 { ("&=" || "^=" || "|=") PARSE_LEVEL2}
	
*/


/*
the bottom 2 (2^0 and 2^1) bits are the base type:
0=unsigned, 
1=signed, 
2=float, 

bit 2^2 designates the pointer level.
bits 2^3 through 2^31 designate the pointer level.
*/

UU vstack_entries[0x4000]; /*stack*/
UU vstack_depth = 0;

/*for higher order operators.*/
void* mallocx(size_t sz){
	char* p = malloc(sz);
	if(!p){
		puts("\nERROR! Malloc Failed!");
		exit(1);
	}
	{size_t i = 0;
		for(;i<sz;i++) p[i]=0;
	}
	return p;
}

static void fail_malloc_print(){
	puts("Failed malloc!");
	exit(1);
}

UU scan_type(char* in, char** out){
	UU basetype = 0;
	UU ptrlevel = 0;
	UU is_lvalue = 0;
	if(*in == 'u') basetype = 0; else
	if(*in == 'i') basetype = 1; else
	if(*in == 'f') basetype = 2; else return 3;
	in++;
	while(*in == '*') {ptrlevel++;in++;}
	*out = in;
	return basetype | (ptrlevel	* 8);
}

/*1 on error.*/
int validate_type(UU type){
	if((type & 3) == 3) return 1;
	return 0;
}

UU type_get_ptr_level(UU type){
	return type/8;
}

UU type_is_lvalue(UU type){
	return ((type/4)&1);
}

UU type_set_lvalue(UU type, UU val){
	if(val)
	type |= 4;
	else
	type &= (~((UU)4));
	return type;
}

UU type_set_ptrlevel(UU type, UU val){
	type &= (1|2|4);
	type |= (8*val);
	return type;
}

UU type_is_ptr(UU type){
	if(type_get_ptr_level) return 1;
	return 0;
}
UU type_get_dereferenced(UU type){
	UU ptrlevel = type_get_ptr_level(type);
	if(ptrlevel == 0) return 3; /*invalid type.*/
	ptrlevel--;
	type &= (1|2|4);
	type += (8*ptrlevel);
	return type;
}
UU type_get_pointer_to(UU type){
	UU ptrlevel = type_get_ptr_level(type);
	if(ptrlevel == 0x10000) return 3; /*invalid type.*/
	ptrlevel++;
	type &= (1|2|4);
	type += (8*ptrlevel);
	return type;
}
UU type_get_basetype(UU t){return t&3;}

#define TOP_TYPE (vstack_entries[vstack_depth-1])
#define ONE_DEEPER_TYPE (vstack_entries[vstack_depth-2])

#define TYPE_FLOAT 2
#define TYPE_SIGNED 1
#define TYPE_UNSIGNED 0
#define TYPE_INVALID 3



static void vstack_push(UU type){
	UU basetype;
	UU is_lvalue;
	UU ptrlevel;
	basetype = type & 3;
	is_lvalue = (type & 4)/4; 
	ptrlevel = type/8;
	if(vstack_depth >= 0x4000)
	{
		puts("<ERROR> expression stack limit reached.");
		exit(1);
	}
	if(basetype != 0 &&
		basetype != 1 &&
		basetype != 2)
	{
		puts("<ERROR> invalid type to push onto vstack.");
		exit(1);
	}
	vstack_entries[vstack_depth++] = type;
}

static UU vstack_pop(){
	if(vstack_depth == 0)
	{
		puts("<ERROR> expression stack empty, cannot pop");
		exit(1);
	}
	vstack_depth--;
	return vstack_entries[vstack_depth];
}

static UU get_real_vstack_depth(){return vstack_depth * 4;}





static void gen_remove_lvalue(char* in, char** target){
	if(vstack_depth < 1){
		printf("<ERROR> tried to remove lvalue when vstack is empty! Here: %s", in);
		exit(1);
	}
	if(!type_is_lvalue(TOP_TYPE)){
		return;
	}
	/*There is an lvalue on the stack to dereference. First, codegen...*/
	*target = strcataf1(*target, "rx0pop;ildrx0_0;rx0push;");
	if(!target)fail_malloc_print();
	
	/*disable lvalue on that entry.*/
	TOP_TYPE = type_set_lvalue(TOP_TYPE, 0);
	return;
}

static void gen_remove_lvalue_deeper(char* in, char** target){
	if(vstack_depth <2){
		printf("<ERROR> tried to remove lvalue 1 deeper when vstack is empty! Here: %s", in);
		exit(1);
	}
	if(!type_is_lvalue(ONE_DEEPER_TYPE)){
		return;
	}
	/*There is an lvalue on the stack to dereference. First, codegen...*/
	*target = strcataf1(*target, "pop %4%;rx0pop;ildrx0_0;rx0push;push %4%;");
	if(!target)fail_malloc_print();
	
	/*disable lvalue on that entry.*/
	ONE_DEEPER_TYPE = type_set_lvalue(ONE_DEEPER_TYPE, 0);
	return;
}

static void gen_dereference(char* in, char** target){
	if(vstack_depth == 0){
		printf("\n<ERROR> tried to dereference pointer when vstack is empty! Here:\n%s\n", in);
		exit(1);
	}
	if(!type_is_ptr(TOP_TYPE)){
		printf("\n<ERROR> tried to dereference pointer on non-pointer! Here: %s", in);
	}
	/*lvalue must be removed to dereference a pointer.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in,target);
	/*There is an lvalue on the stack to dereference. First, codegen...*/
	*target = strcataf1(*target, "rx0pop;ildrx0_0;rx0push;");
	if(!target)fail_malloc_print();
	
	/*reduce pointer level*/
	TOP_TYPE = type_get_dereferenced(TOP_TYPE);
	return;
}


static void gen_unary_plus(char* in, char** target){
	if(vstack_depth < 1){
		printf("<ERROR> unary + has not enough arguments.: %s", in);
		exit(1);
	}
	return; /*literally do nothing*/
}

static void gen_unary_minus(char* in, char** target){
	UU bt;
	if(vstack_depth < 1){
		printf("\n<ERROR> unary - has not enough arguments.:\n%s\n", in);
		exit(1);
	}
	if(type_is_ptr(TOP_TYPE) ){
		printf("\n<ERROR> unary - is not compatible with pointers.: %s", in);
		exit(1);
	}
	if(type_is_lvalue(TOP_TYPE))
		gen_remove_lvalue(in, target);
	
	bt = TOP_TYPE & 3;
	if(bt == TYPE_UNSIGNED || bt == TYPE_SIGNED)
	{
		*target = strcataf1(*target, "rx0pop;rxcompl;rxincr;rx0push;");
		if(!target)fail_malloc_print();
		/*the new entry is a SIGNED INTEGER.*/
		TOP_TYPE = TYPE_SIGNED;
	} else if(bt == TYPE_FLOAT){
		*target = strcataf1(*target, "rx0pop;lrx1 %?-1%;fltmul;rx0push;");
	} else {
		printf("\n<ERROR> unary - on bad type 3,\n %s\n",in);
		exit(1);
	}
	return;
}

/*WARNING! this removes lvalue!*/
static void gen_type_conversion(UU target_type, char* in, char** target, char is_explicit)
{
	if(vstack_depth < 1){
		printf("\n<ERROR> type conversion has not enough arguments.: %s", in);
		exit(1);
	}
	if(type_is_lvalue(target_type)){
		printf("\n<ERROR> cannot convert to an lvalue. %s", in);
		exit(1);
	}
	if(type_get_basetype(target_type) == 3){
		printf("\n<ERROR> cannot convert to a bad type! here:\n%s\n",in);
		exit(1);
	}
	/*type conversion always implies removing lvalue.*/
	if(type_is_lvalue(TOP_TYPE)){
		gen_remove_lvalue(in, target);
	}

	if(target_type == TOP_TYPE) return; /*Nothing needs to be done.*/
	
	if(type_is_ptr(target_type)){ /*converting to a pointer?*/
		if(type_is_ptr(TOP_TYPE)){
			/*Pointer to pointer conversion is allowed without any errors.*/
			TOP_TYPE = target_type;
			return;
		}
		/*Float?*/
		if((TOP_TYPE) == TYPE_FLOAT){
			printf("\n<ERROR> cannot convert float to pointer, here:\n%s\n",in);
			exit(1);
		}
		if(is_explicit){
			if((TOP_TYPE) == TYPE_UNSIGNED){
				TOP_TYPE = target_type;
				return;
			}
			if((TOP_TYPE) == TYPE_SIGNED){
				TOP_TYPE = target_type;
				return;
			}
		} else {
			if((TOP_TYPE) == TYPE_UNSIGNED){
				printf("\n<ERROR> cannot implicitly convert unsigned integer to pointer, here:\n%s\n",in);
				exit(1);
			}
			if((TOP_TYPE) == TYPE_SIGNED){
				printf("\n<ERROR> cannot implicitly convert signed integer to pointer, here:\n%s\n",in);
				exit(1);
			}
		}
		goto error;
	} else if((target_type) == TYPE_FLOAT){ /*Converting to float?*/
		/*converting to a pointer?*/
		if(type_is_ptr(TOP_TYPE)){
			/*pointer to float is not allowed*/
			printf("\n<ERROR> cannot convert pointer to float!, here:\n%s\n",in);
			exit(1);
		}
		/*Float? same type as we started with.*/
		if((TOP_TYPE) == TYPE_FLOAT){return;}

		/*integer to float conversion*/
		if(
			((TOP_TYPE) == TYPE_UNSIGNED) ||
			((TOP_TYPE) == TYPE_SIGNED)
		){
			/*Unsigned Integer to float conversion is allowed without any errors.*/
			*target = strcataf1(*target, "rx0pop;rxitof;rx0push;");
			TOP_TYPE = target_type;
			return;
		}
		goto error;
	} else if(target_type == TYPE_UNSIGNED){ /*Converting to unsigned int?*/
		if(type_is_ptr(TOP_TYPE)){
			/*Pointer to unsigned int conversion must be explicit*/
			if(is_explicit){
				TOP_TYPE = target_type;
				return;
			} 
			printf("\n<ERROR> Cannot implicitly convert pointer to unsigned int. here:\n%s\n",in);
			exit(1);

		}
		if((TOP_TYPE) == TYPE_UNSIGNED){
			TOP_TYPE = target_type;
			return;
		}
		if((TOP_TYPE) == TYPE_SIGNED){
			TOP_TYPE = target_type;
			return;
		}
		/*From float*/
		if((TOP_TYPE) == TYPE_FLOAT){
			*target = strcataf1(*target, "rx0pop;rxftoi;rx0push;");
			TOP_TYPE = target_type;
			return;
		}
		goto error;
	} else if(target_type == TYPE_SIGNED){ /*Converting to signed int?*/
		/*From a pointer.*/
		if(type_is_ptr(TOP_TYPE)){
			/*Pointer to unsigned int conversion must be explicit*/
			if(is_explicit){
				TOP_TYPE = target_type;
				return;
			}
			printf("\n<ERROR> Cannot implicitly convert pointer to signed int. here:\n%s\n",in);
			exit(1);
		}
		/*From another integer type*/
		if((TOP_TYPE) == TYPE_UNSIGNED){
			TOP_TYPE = target_type;
			return;
		}
		if((TOP_TYPE) == TYPE_SIGNED){
			TOP_TYPE = target_type;
			return;
		}
		/*From float*/
		if((TOP_TYPE) == TYPE_FLOAT){
			*target = strcataf1(*target, "rx0pop;rxftoi;rx0push;");
			TOP_TYPE = target_type;
			return;
		}
		goto error;
	} else if((target_type) == 3){ /*badtype*/
		printf("\n<ERROR> Cannot convert to bad type! here:\n%s\n",in);
		exit(1);
	}
	error:;
	printf("\n<ERROR> Cannot do a requested type conversion here:\n%s\n",in);
	exit(1);
}


/*WARNING! this removes lvalue!*/
static void gen_type_conversion_deeper(UU target_type, char* in, char** target, char is_explicit)
{
	if(vstack_depth < 2){
		printf("\n<ERROR> deeper type conversion has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	if(type_is_lvalue(target_type)){
		printf("\n<ERROR> cannot convert to an lvalue. here:\n%s\n", in);
		exit(1);
	}
	if(type_get_basetype(target_type) == 3){
		printf("\n<ERROR> cannot convert to a bad type! here:\n%s\n",in);
		exit(1);
	}
	/*type conversion always implies removing lvalue.*/
	if(type_is_lvalue(ONE_DEEPER_TYPE)){
		gen_remove_lvalue_deeper(in, target);
	}

	if((ONE_DEEPER_TYPE) == TYPE_INVALID){
		printf("\n<ERROR> Cannot convert invalid type! Here: %s", in);
		exit(1);
	}

	if(target_type == ONE_DEEPER_TYPE) return; /*Nothing needs to be done.*/
	
	if(type_is_ptr(target_type)){ /*converting to a pointer?*/
		if(type_is_ptr(ONE_DEEPER_TYPE)){
			/*Pointer to pointer conversion is allowed without any errors.*/
			ONE_DEEPER_TYPE = target_type;
			return;
		}
		/*Float?*/
		if(ONE_DEEPER_TYPE == TYPE_FLOAT){
			printf("\n<ERROR> cannot convert float to pointer, here:\n%s\n",in);
			exit(1);
		}
		if(is_explicit){
			if(ONE_DEEPER_TYPE == TYPE_UNSIGNED){
				ONE_DEEPER_TYPE = target_type;
				return;
			}
			if(ONE_DEEPER_TYPE == TYPE_SIGNED){
				ONE_DEEPER_TYPE = target_type;
				return;
			}
		} else {
			if(ONE_DEEPER_TYPE == TYPE_UNSIGNED){
				printf("\n<ERROR> cannot implicitly convert unsigned integer to pointer, here:\n%s\n",in);
				exit(1);
			}
			if(ONE_DEEPER_TYPE == TYPE_SIGNED){
				printf("\n<ERROR> cannot implicitly convert signed integer to pointer, here:\n%s\n",in);
				exit(1);
			}
		}
		goto error;
	} else if((target_type) == TYPE_FLOAT){ /*Converting to float?*/
		/*converting to a pointer?*/
		if(type_is_ptr(ONE_DEEPER_TYPE)){
			/*pointer to float is not allowed*/
			printf("\n<ERROR> cannot convert pointer to float!, here:\n%s\n",in);
			exit(1);
		}
		/*Float? same type as we started with.*/
		if(ONE_DEEPER_TYPE == TYPE_FLOAT){return;}

		/*integer to float conversion*/
		if(
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED) ||
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		){
			/*Any Integer to float conversion is allowed without any errors.*/
			*target = strcataf1(*target, "pop %4%;rx0pop;rxitof;rx0push;push %4%;");
			ONE_DEEPER_TYPE = target_type;
			return;
		}
		goto error;
	} else if((target_type) == TYPE_UNSIGNED){ /*Converting to unsigned int?*/
		if(type_is_ptr(ONE_DEEPER_TYPE)){
			/*Pointer to unsigned int conversion must be explicit*/
			if(is_explicit){
				ONE_DEEPER_TYPE = target_type;
				return;
			} 
			printf("\n<ERROR> Cannot implicitly convert pointer to unsigned int. here:\n%s\n",in);
			exit(1);

		}
		if(ONE_DEEPER_TYPE == TYPE_UNSIGNED){
			ONE_DEEPER_TYPE = target_type;
			return;
		}
		if(ONE_DEEPER_TYPE == TYPE_SIGNED){
			ONE_DEEPER_TYPE = target_type;
			return;
		}
				/*float to integer conversion*/
		if(ONE_DEEPER_TYPE == TYPE_FLOAT){
			*target = strcataf1(*target, "pop %4%;rx0pop;rxftoi;rx0push;push %4%;");
			ONE_DEEPER_TYPE = target_type;
			return;
		}
		goto error;
	} else if((target_type) == TYPE_SIGNED){ /*Converting to signed int?*/
		if(type_is_ptr(ONE_DEEPER_TYPE)){
			/*Pointer to unsigned int conversion must be explicit*/
			if(is_explicit){
				ONE_DEEPER_TYPE = target_type;
				return;
			}
			printf("\n<ERROR> Cannot implicitly convert pointer to signed int. here:\n%s\n",in);
			exit(1);

		}
		/*From another integer type.*/
		if(ONE_DEEPER_TYPE == TYPE_UNSIGNED){
			ONE_DEEPER_TYPE = target_type;
			return;
		}
		if(ONE_DEEPER_TYPE == TYPE_SIGNED){
			ONE_DEEPER_TYPE = target_type;
			return;
		}
		/*From float.*/
		if(ONE_DEEPER_TYPE == TYPE_FLOAT){
			*target = strcataf1(*target, "pop %4%;rx0pop;rxftoi;rx0push;push %4%;");
			ONE_DEEPER_TYPE = target_type;
			return;
		}
	} else if(type_get_basetype(target_type) == TYPE_INVALID){ /*badtype*/
		printf("\n<ERROR> Cannot convert to bad type! here:\n%s\n",in);
		exit(1);
	}
	error:;
	printf("\n<ERROR> Cannot do a requested type conversion here:\n%s\n",in);
	exit(1);
}

static void gen_add(char* in, char** target){
	UU final_type = 0; /**/
	if(vstack_depth < 2){
		printf("\n<ERROR> add has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*check for pointer plus pointer, which is erroneous.*/
	if(
		type_is_ptr(TOP_TYPE) &&
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> adding two pointers? here:\n%s\n", in);
		exit(1);
	}
	/*check for invalid types*/
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> adding invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and only one of them can be a pointer...*/
	/*? + ptr*/
	if(type_is_ptr(TOP_TYPE)){
		/*the pointer is on top, something else is below...*/
		if(
			ONE_DEEPER_TYPE == TYPE_UNSIGNED ||
			ONE_DEEPER_TYPE == TYPE_SIGNED
		){
			UU new_type;
			new_type = vstack_pop(); /*The top element is the pointer type we want to be!*/
			vstack_pop();
			vstack_push(new_type);
			*target = strcataf1(*target, "rx2pop;rx0pop;lb 2;rx1b;rxlsh;rx1_2;rxadd;rx0push;");
			return;
		}
		/*Can't add a float to a pointer.*/
		if((ONE_DEEPER_TYPE) == TYPE_FLOAT){
			printf("\n<ERROR> Cannot generate add for float plus pointer here\n%s\n",in);
			exit(1);
		}
		goto error;
	}
	/*ptr + ?*/
	if(type_is_ptr(ONE_DEEPER_TYPE)){
		/*the pointer is below, something else is on top...*/
		if(
			TOP_TYPE == TYPE_UNSIGNED ||
			TOP_TYPE == TYPE_SIGNED
		){
			UU new_type;
			vstack_pop();
			new_type = vstack_pop(); /*The second element is the pointer type we want to be!*/
			vstack_push(new_type);
			*target = strcataf1(*target, "rx0pop;lb 2;rx1b;rxlsh;rx1pop;rxadd;rx0push;");
			return;
		}
		/*Can't add a float to a pointer.*/
		if((TOP_TYPE) == TYPE_FLOAT){
			printf("\n<ERROR> Cannot generate add for float plus pointer here\n%s\n",in);
			exit(1);
		}
		goto error;
	}

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)
	)
	{
		UU new_type;
		new_type = vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rx1pop;rxadd;rx0push;");
		return;
	}
	/*signed plus unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rx1pop;rxadd;rx0push;");
		return;
	}
	/*two floats?*/
	if(
		((TOP_TYPE) == TYPE_FLOAT) &&
		((ONE_DEEPER_TYPE) == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;fltadd;rx0push;");
		return;
	}
	/*float+int*/
	if(
		((TOP_TYPE == TYPE_SIGNED) || (TOP_TYPE == TYPE_UNSIGNED)) &&
		((ONE_DEEPER_TYPE == TYPE_FLOAT))
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1pop;fltadd;rx0push;");
		return;
	}
	/*int+float*/
	if(
		((ONE_DEEPER_TYPE == TYPE_SIGNED) || (ONE_DEEPER_TYPE == TYPE_UNSIGNED)) &&
		((TOP_TYPE == TYPE_FLOAT))
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltadd;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate add at \n%s\n",in);
	exit(1);
}

static void gen_sub(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> sub has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);

	/*There are henceforth no lvalues!*/
	
	/*check for pointer minus pointer, which is only valid if the two types are the same.*/
	if(
		type_is_ptr(TOP_TYPE) &&
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		UU new_type;
		if(TOP_TYPE != ONE_DEEPER_TYPE){
			printf("\n<ERROR> subtracting two pointers of different types? here:\n%s\n", in);
			exit(1);
		}
		new_type = vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxsub;rx0push;");
		return;
	}
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> subtracting invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and only one of them can be a pointer...*/
	/*? - ptr*/
	if(type_is_ptr(TOP_TYPE)){
		/*the pointer is on top, something else is below...*/
		if(
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED) ||
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		){
			printf("\n<ERROR> Cannot subtract pointer from integer. here\n%s\n",in);
			exit(1);
		}
		/*Can't add a float to a pointer.*/
		if((ONE_DEEPER_TYPE) == TYPE_FLOAT){
			printf("\n<ERROR> Cannot generate sub for float - pointer here\n%s\n",in);
			exit(1);
		}
		goto error;
	}
	/*ptr - ?*/
	if(type_is_ptr(ONE_DEEPER_TYPE)){
		/*the pointer is below, something else is on top...*/
		if(
			(TOP_TYPE == TYPE_UNSIGNED) ||
			(TOP_TYPE == TYPE_SIGNED)
		){
			UU new_type;
			vstack_pop();
			new_type = vstack_pop();
			vstack_push(new_type);
			*target = strcataf1(*target, "rx0pop;lb 2;rx1b;rxlsh;rx1_0;rx0pop;rxsub;rx0push;");
			return;
		}
		/*Can't add a float to a pointer.*/
		if((TOP_TYPE) == TYPE_FLOAT){
			printf("\n<ERROR> Cannot generate sub for pointer - float here\n%s\n",in);
			exit(1);
		}
		goto error;
	}

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(
		(
			((TOP_TYPE == TYPE_SIGNED)) &&
			((ONE_DEEPER_TYPE == TYPE_SIGNED))
		)||(
			((TOP_TYPE == TYPE_UNSIGNED)) &&
			((ONE_DEEPER_TYPE == TYPE_UNSIGNED))
		)
	)
	{
		UU new_type;
		new_type = vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxsub;rx0push;");
		return;
	}

	/*signed plus unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxsub;rx0push;");
		return;
	}
	/*two floats?*/
	if(
		(TOP_TYPE == TYPE_FLOAT) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;fltsub;rx0push;");
		return;
	}
	/*float - int*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED)
			|| 
			(TOP_TYPE == TYPE_UNSIGNED)
		) &&
		((ONE_DEEPER_TYPE) == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1_0;rx0pop;fltsub;rx0push;");
		return;
	}
	/*int - float*/
	if(
		(
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
			|| 
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		) &&
		((TOP_TYPE) == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltsub;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate sub at \n%s\n",in);
	exit(1);
}

static void gen_mul(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> mul has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> multiplying pointers? here:\n%s\n", in);
		exit(1);
	}
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> multiplying invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		) || (
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)
	)
	{
		UU new_type;
		new_type = vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rx1pop;rxmul;rx0push;");
		return;
	}
	/*signed times unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rx1pop;rxmul;rx0push;");
		return;
	}
	/*two floats?*/
	if(
		(TOP_TYPE == TYPE_FLOAT) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rx1pop;fltmul;rx0push;");
		return;
	}
	/* float * int */
	if(
		(
			(TOP_TYPE == TYPE_SIGNED)
			|| 
			(TOP_TYPE == TYPE_UNSIGNED)
		) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1pop;fltmul;rx0push;");
		return;
	}
	/* int * float */
	if(
		(
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
			|| 
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		) &&
		((TOP_TYPE) == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltmul;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate sub at \n%s\n",in);
	exit(1);
}


static void gen_div(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> div has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> dividing pointers? here:\n%s\n", in);
		exit(1);
	}
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> dividing invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(
		((TOP_TYPE) == TYPE_SIGNED) &&
		((ONE_DEEPER_TYPE) == TYPE_SIGNED)
	)
	{
		UU new_type= TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxidiv;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxdiv;rx0push;");
		return;
	}
	/*signed divided by unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxidiv;rx0push;");
		return;
	}
	/*two floats?*/
	if(
		(TOP_TYPE == TYPE_FLOAT) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;fltdiv;rx0push;");
		return;
	}
	/*float / int*/
	if(
		((TOP_TYPE == TYPE_SIGNED) ||(TOP_TYPE == TYPE_UNSIGNED))&&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1_0;rx0pop;fltdiv;rx0push;");
		return;
	}
	/*int / float*/
	if(
		((ONE_DEEPER_TYPE == TYPE_SIGNED) ||(ONE_DEEPER_TYPE == TYPE_UNSIGNED)) &&
		(TOP_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_FLOAT;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltdiv;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate div at \n%s\n",in);
	exit(1);
}


static void gen_mod(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> modulo has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> modulo on pointers? here:\n%s\n", in);
		exit(1);
	}
	if(
		(TOP_TYPE == TYPE_FLOAT) ||
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		printf("\n<ERROR> modulo on floats? here:\n%s\n", in);
		exit(1);
	}
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> modulo on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		((TOP_TYPE) == TYPE_SIGNED) &&
		((ONE_DEEPER_TYPE) == TYPE_SIGNED)
	)
	{
		UU new_type;
		new_type = vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rximod;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type;
		new_type = vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxmod;rx0push;");
		return;
	}
	/*signed divided by unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rximod;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate mod at \n%s\n",in);
	exit(1);
}


static void gen_lsh(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> left shift has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> left shift on pointers? here:\n%s\n", in);
		exit(1);
	}
	if(
		(TOP_TYPE == TYPE_FLOAT) ||
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		printf("\n<ERROR> left shift on floats? here:\n%s\n", in);
		exit(1);
	}
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> left shift on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxlsh;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxlsh;rx0push;");
		return;
	}
	/*signed left shifted by unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxlsh;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate left shift at \n%s\n",in);
	exit(1);
}

static void gen_rsh(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> right shift has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> right shift on pointers? here:\n%s\n", in);
		exit(1);
	}
	if(
		(TOP_TYPE == TYPE_FLOAT) ||
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		printf("\n<ERROR> right shift on floats? here:\n%s\n", in);
		exit(1);
	}
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> right shift on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxrsh;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxrsh;rx0push;");
		return;
	}
	/*signed right shifted by unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxrsh;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate right shift at \n%s\n",in);
	exit(1);
}

static void gen_bitand(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> bitand has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> bitand on pointers? here:\n%s\n", in);
		exit(1);
	}
	if(
		(TOP_TYPE == TYPE_FLOAT) ||
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		printf("\n<ERROR> bitand on floats? here:\n%s\n", in);
		exit(1);
	}
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> bitand on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxand;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxand;rx0push;");
		return;
	}
	/*signed right shifted by unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxand;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate bitand at \n%s\n",in);
	exit(1);
}


static void gen_bitor(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> bitor has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> bitor on pointers? here:\n%s\n", in);
		exit(1);
	}
	if(
		(TOP_TYPE == TYPE_FLOAT) ||
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		printf("\n<ERROR> bitor on floats? here:\n%s\n", in);
		exit(1);
	}
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> bitor on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxor;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxor;rx0push;");
		return;
	}
	/*signed right shifted by unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxor;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate bitor at \n%s\n",in);
	exit(1);
}

static void gen_bitxor(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> bitxor has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> bitxor on pointers? here:\n%s\n", in);
		exit(1);
	}
	if(
		(TOP_TYPE == TYPE_FLOAT) ||
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		printf("\n<ERROR> bitxor on floats? here:\n%s\n", in);
		exit(1);
	}
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> bitxor on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxxor;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxxor;rx0push;");
		return;
	}
	/*signed right shifted by unsigned or vice versa? The result is signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_SIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxxor;rx0push;");
		return;
	}

	error:;
	printf("\n<ERROR> Cannot generate bitxor at \n%s\n",in);
	exit(1);
}

static void gen_lt(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> less than has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> less than on pointers? here:\n%s\n", in);
		exit(1);
	}

	
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> less than on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 0;cmp;lb 1;and;arx0;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxcmp;lb 0;cmp;lb 1;and;arx0;rx0push;");
		return;
	}
	/*signed compared to unsigned? Pretend each are signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 0;cmp;lb 1;and;arx0;rx0push;");
		return;
	}

	/*float < float*/
	if(
		(TOP_TYPE == TYPE_FLOAT) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;fltcmp;lb 0;cmp;lb 1;and;arx0;rx0push;");
		return;
	}

	/*float<signed*/
	if(
		( (TOP_TYPE == TYPE_SIGNED)  || (TOP_TYPE == TYPE_UNSIGNED) ) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1_0;rx0pop;fltcmp;lb 0;cmp;lb 1;and;arx0;rx0push;");
		return;
	}

		/*signed<float*/
	if(
		( (ONE_DEEPER_TYPE == TYPE_SIGNED)  || (ONE_DEEPER_TYPE == TYPE_UNSIGNED) ) &&
		(TOP_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltcmp;lb 0;cmp;lb 1;and;arx0;rx0push;");
		return;
	}


	error:;
	printf("\n<ERROR> Cannot generate less than at \n%s\n",in);
	exit(1);
}



static void gen_gt(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> greater than has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> greater than on pointers? here:\n%s\n", in);
		exit(1);
	}

	
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> greater than on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 2;cmp;arx0;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxcmp;lb 2;cmp;arx0;rx0push;");
		return;
	}
	/*signed compared to unsigned? Pretend each are signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 2;cmp;arx0;rx0push;");
		return;
	}

	/*float > float*/
	if(
		(TOP_TYPE == TYPE_FLOAT) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;fltcmp;lb 2;cmp;arx0;rx0push;");
		return;
	}

	/*float>signed*/
	if(
		( (TOP_TYPE == TYPE_SIGNED)  || (TOP_TYPE == TYPE_UNSIGNED) ) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1_0;rx0pop;fltcmp;lb 2;cmp;arx0;rx0push;");
		return;
	}

		/*signed>float*/
	if(
		( (ONE_DEEPER_TYPE == TYPE_SIGNED)  || (ONE_DEEPER_TYPE == TYPE_UNSIGNED) ) &&
		(TOP_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltcmp;lb 2;cmp;arx0;rx0push;");
		return;
	}


	error:;
	printf("\n<ERROR> Cannot generate greater than at \n%s\n",in);
	exit(1);
}

static void gen_lte(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> less than or equal has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> less than or equal on pointers? here:\n%s\n", in);
		exit(1);
	}

	
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> less than or equal on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 2;cmp;nota;arx0;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxcmp;lb 2;cmp;nota;arx0;rx0push;");
		return;
	}
	/*signed compared to unsigned? Pretend each are signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 2;cmp;nota;arx0;rx0push;");
		return;
	}

	/*float > float*/
	if(
		(TOP_TYPE == TYPE_FLOAT) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;fltcmp;lb 2;cmp;nota;arx0;rx0push;");
		return;
	}

	/*float>signed*/
	if(
		( (TOP_TYPE == TYPE_SIGNED)  || (TOP_TYPE == TYPE_UNSIGNED) ) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1_0;rx0pop;fltcmp;lb 2;cmp;nota;arx0;rx0push;");
		return;
	}

		/*signed>float*/
	if(
		( (ONE_DEEPER_TYPE == TYPE_SIGNED)  || (ONE_DEEPER_TYPE == TYPE_UNSIGNED) ) &&
		(TOP_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltcmp;lb 2;cmp;nota;arx0;rx0push;");
		return;
	}


	error:;
	printf("\n<ERROR> Cannot less than or equal at \n%s\n",in);
	exit(1);
}

static void gen_gte(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> greater than or equals has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> greater than or equals on pointers? here:\n%s\n", in);
		exit(1);
	}

	
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> greater than on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;boolify;arx0;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxcmp;boolify;arx0;rx0push;");
		return;
	}
	/*signed compared to unsigned? Pretend each are signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;boolify;arx0;rx0push;");
		return;
	}

	/*float >= float*/
	if(
		(TOP_TYPE == TYPE_FLOAT) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;fltcmp;boolify;arx0;rx0push;");
		return;
	}

	/*float>=signed*/
	if(
		( (TOP_TYPE == TYPE_SIGNED)  || (TOP_TYPE == TYPE_UNSIGNED) ) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1_0;rx0pop;fltcmp;boolify;arx0;rx0push;");
		return;
	}

		/*signed>=float*/
	if(
		( (ONE_DEEPER_TYPE == TYPE_SIGNED)  || (ONE_DEEPER_TYPE == TYPE_UNSIGNED) ) &&
		(TOP_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltcmp;boolify;arx0;rx0push;");
		return;
	}


	error:;
	printf("\n<ERROR> Cannot generate greater than or equals at \n%s\n",in);
	exit(1);
}

static void gen_eq(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> equals has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxcmp;lb 1;and;arx0;rx0push;");
		return;
	}

	
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> equals on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 1;and;arx0;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxcmp;lb 1;and;arx0;rx0push;");
		return;
	}
	/*signed compared to unsigned? Pretend each are signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 1;and;arx0;rx0push;");
		return;
	}

	/*float >= float*/
	if(
		(TOP_TYPE == TYPE_FLOAT) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;fltcmp;lb 1;and;arx0;rx0push;");
		return;
	}

	/*float>=signed*/
	if(
		( (TOP_TYPE == TYPE_SIGNED)  || (TOP_TYPE == TYPE_UNSIGNED) ) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1_0;rx0pop;fltcmp;lb 1;and;arx0;rx0push;");
		return;
	}

		/*signed>=float*/
	if(
		( (ONE_DEEPER_TYPE == TYPE_SIGNED)  || (ONE_DEEPER_TYPE == TYPE_UNSIGNED) ) &&
		(TOP_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltcmp;lb 1;and;arx0;rx0push;");
		return;
	}


	error:;
	printf("\n<ERROR> Cannot generate equals at \n%s\n",in);
	exit(1);
}



static void gen_neq(char* in, char** target){
	if(vstack_depth < 2){
		printf("\n<ERROR> not equals has not enough arguments. here:\n%s\n", in);
		exit(1);
	}
	/*1) remove lvalue on each. We don't need lvalues to add.*/
	if(type_is_lvalue(TOP_TYPE)) gen_remove_lvalue(in, target);
	if(type_is_lvalue(ONE_DEEPER_TYPE)) gen_remove_lvalue_deeper(in, target);
	/*2) what type are we doing*/
	/*Can't multiply with pointers!*/
	if(
		type_is_ptr(TOP_TYPE) ||
		type_is_ptr(ONE_DEEPER_TYPE)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxcmp;lb 1;and;nota;arx0;rx0push;");
		return;
	}

	
	if(
		validate_type(TOP_TYPE) ||
		validate_type(ONE_DEEPER_TYPE)
	){
		printf("\n<ERROR> not equals on invalid types? here:\n%s\n", in);
		exit(1);
	}
	/*From here on, there are no Lvalues, and no pointers either!...*/

	/*We have an ordinary math operation. What is our final type?*/
	/*Handle the easy cases first- two unsigned integers and two signed integers.*/
	if(

		(TOP_TYPE == TYPE_SIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_SIGNED)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 1;and;nota;arx0;rx0push;");
		return;
	}

	/*unsigned variant.*/
	if(
		(TOP_TYPE == TYPE_UNSIGNED) &&
		(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxcmp;lb 1;and;nota;arx0;rx0push;");
		return;
	}
	/*signed compared to unsigned? Pretend each are signed.*/
	if(
		(
			(TOP_TYPE == TYPE_SIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_UNSIGNED)
		)||(
			(TOP_TYPE == TYPE_UNSIGNED) &&
			(ONE_DEEPER_TYPE == TYPE_SIGNED)
		)
	)
	{
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxicmp;lb 1;and;nota;arx0;rx0push;");
		return;
	}

	/*float >= float*/
	if(
		(TOP_TYPE == TYPE_FLOAT) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;fltcmp;lb 1;and;nota;arx0;rx0push;");
		return;
	}

	/*float>=signed*/
	if(
		( (TOP_TYPE == TYPE_SIGNED)  || (TOP_TYPE == TYPE_UNSIGNED) ) &&
		(ONE_DEEPER_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx0pop;rxitof;rx1_0;rx0pop;fltcmp;lb 1;and;nota;arx0;rx0push;");
		return;
	}

		/*signed>=float*/
	if(
		( (ONE_DEEPER_TYPE == TYPE_SIGNED)  || (ONE_DEEPER_TYPE == TYPE_UNSIGNED) ) &&
		(TOP_TYPE == TYPE_FLOAT)
	){
		UU new_type = TYPE_UNSIGNED;
		vstack_pop();
		vstack_pop();
		vstack_push(new_type);
		*target = strcataf1(*target, "rx1pop;rx0pop;rxitof;fltcmp;lb 1;and;arx0;rx0push;");
		return;
	}


	error:;
	printf("\n<ERROR> Cannot generate equals at \n%s\n",in);
	exit(1);
}


static long scan_signed(char* in, char** out){
	return strtol(in, out, 0);
}

static unsigned long scan_unsigned(char* in, char** out){
	return strtoul(in, out, 0);
}

static float scan_float(char* in, char** out){
	return strtof(in, out);
}

static UU try_is_signed(char* in){
	char* out;
	UU retval = 0;
	scan_signed(in, &out);
	if(out == in) return 0;
	
	/*return the difference.*/
	while(in != out) {in++; retval++;}
	return retval;

}

static UU try_is_unsigned(char* in){
	char* out;
	UU retval = 0;
	scan_unsigned(in, &out);
	if(out == in) return 0;
	
	/*return the difference.*/
	while(in != out) {in++; retval++;}
	return retval;

}

static UU try_is_float(char* in){
	char* out;
	UU retval = 0;
	scan_unsigned(in, &out);
	if(out == in) return 0;
	
	/*return the difference.*/
	while(in != out) {in++; retval++;}
	return retval;
}

static char* parse_number(char* in, char** target){
	char* out;
	char buf_codegen[128]; /*code generated.*/
	while(isspace(in)) in++;
	if(in[0] == '\0') goto error;
	UU difu, difi, diff;

	difu = try_is_unsigned(in);
	difi = try_is_signed(in);
	diff = try_is_float(in);

	if(diff)
		if(diff > difi)
		{
			UU q;
			float temp;
			/*unsigned number.*/
			temp = scan_float(in, &in);
			sprintf(buf_codegen,"lrx0 %%?%f%%;rx0push;", temp);
			/*append to target*/
			*target = strcataf1(*target, buf_codegen);
			vstack_push(2);
			return in;
		}
	/*using the integer scanner made a bigger difference.*/
	if(difi > difu){
		SUU temp;
		/*unsigned number.*/
		temp = scan_signed(in, &in);
		/*push it onto the stack.*/
		sprintf(buf_codegen,"lrx0 %%/%ld%%;rx0push;", (long)temp);
		/*append to target*/
		*target = strcataf1(*target, buf_codegen);
		vstack_push(1);
		return in;
	}
	if(difu){
		UU q;
		/*unsigned number.*/
		q = scan_unsigned(in, &in);
		sprintf(buf_codegen,"lrx0 %%/%lu%%;rx0push;", (unsigned long)q);
		/*append to target*/
		*target = strcataf1(*target, buf_codegen);
		vstack_push(0);
		return in;
	}
	error:
	printf("\n<EXPRESSION PARSING ERROR> number %s failed to parse.\n", in);
	exit(1);
}

static char* parse_ident(char* in, char** target){
	char buf_codegen[64];
	char is_stack_or_global = 0; /*0 = stack, 1 = global. $ means stack.*/
	UU stack_depth;
	UU basetype = 0;
	UU ptrlevel = 0;

	while(isspace(in)) in++;

	if(*in != '$') goto error;
	in++;
	if(*in == 'u') basetype = 0; else
	if(*in == 'i') basetype = 1; else
	if(*in == 'f') basetype = 2; else goto error;
	in++;
	while(*in == '*'){in++;ptrlevel++;}
	if(in[0] == '['/*]*/) {is_stack_or_global = 0;} else
	if(in[0] == '('/*)*/) {is_stack_or_global = 1;} else goto error;
	in++;
	if(!try_is_unsigned(in)) goto error;
	stack_depth = scan_unsigned(in, &in) + get_real_vstack_depth();
	if(is_stack_or_global == 0)if(in[0] != /*[*/']') goto error; /*Does not end in close parentheses!*/
	if(is_stack_or_global == 1)if(in[0] != /*(*/')') goto error; /*Does not end in close parentheses!*/

	sprintf(buf_codegen,"astp;llb %%%lu%%;sub;rx0a;rx0push;",stack_depth);
	vstack_push( (basetype & 3) | 4 | (ptrlevel * 8)  );
	return in;
	error:
	printf("\n<EXPRESSION PARSING ERROR> identifier %s failed to parse.\n", in);
	exit(1);
}


char* PARSE_LEVEL_PAREN(char* in, char** target);
/*left to right*/
char* PARSE_LEVEL_BRACK(char* in, char** target);
/*right to left*/
char* PARSE_LEVEL_UNARY(char* in, char** target);
/*left to right*/
char* PARSE_LEVEL_MULDIVMOD(char* in, char** target);
char* PARSE_LEVEL_ADDSUB(char* in, char** target);
char* PARSE_LEVEL_SHIFTS(char* in, char** target);
char* PARSE_LEVEL_LT_LTE(char* in, char** target);
char* PARSE_LEVEL_GT_GTE(char* in, char** target);
char* PARSE_LEVEL_EQ_NEQ(char* in, char** target);
char* PARSE_LEVEL_BITAND(char* in, char** target);
char* PARSE_LEVEL_BITXOR(char* in, char** target);
char* PARSE_LEVEL_BITOR(char* in, char** target);
/*right to left.*/
char* PARSE_LEVEL_ASGN(char* in, char** target);



char* PARSE_LEVEL_IDENT_OR_NUMBER(char* in, char** target){
	while(isspace(*in))in++;
	if(*in == '\\' || *in == '$') return parse_ident(in, target);
	return parse_number(in, target);
}

char* PARSE_LEVEL_PAREN(char* in, char** target){
	UU saved_vstack_depth;
	char* in_orig = in;
	saved_vstack_depth = vstack_depth;
	while(isspace(in)) in++;
	if(in[0] != '(') return PARSE_LEVEL_IDENT_OR_NUMBER(in, target);
	/*This is a parenthesized expression.*/
	in = PARSE_LEVEL_ASGN(in, target);
	while(isspace(in)) in++;
	if(in[0] != ')') {
		printf("\n<EXPRESSION PARSING ERROR> parenthesized expression '%s' not terminated with ending parentheses.",in_orig);
		exit(1);
	}
	in++; /*consume the close parentheses.*/
	if(vstack_depth != (saved_vstack_depth + 1)){
		printf("<EXPRESSION PARSING ERROR> parenthesized expression '%s' does not increase vstack depth by one!", in_orig);
		exit(1);
	}
	return in;
}

char* PARSE_LEVEL_BRACK(char* in, char** target){
	UU saved_vstack_depth;
	char* in_orig = in;
	saved_vstack_depth = vstack_depth;
	while(isspace(in)) in++;
	
	in = PARSE_LEVEL_PAREN(in, target);
	while(isspace(in)) in++;
	if(in[0] != '[') return in;
	in++;
	/*we have a bracketed expression!*/
	if(vstack_depth != (saved_vstack_depth+1)){
		printf("<EXPRESSION PARSING ERROR> expression '%s' with postfix brackets does not increase vstack depth by one!", in_orig);
		exit(1);
	}
	in = PARSE_LEVEL_ASGN(in, target);
	while(isspace(in)) in++;
	if(in[0] != ']') {
		printf("<EXPRESSION PARSING ERROR> expression '%s' with postfix brackets is not ended with a closing bracket.", in_orig);
		exit(1);
	}
	in++;
	return in;
}

char* PARSE_LEVEL_UNARY(char* in, char** target){
	UU saved_vstack_depth;
	char* in_orig = in;
	saved_vstack_depth = vstack_depth;
	while(isspace(in)) in++;
	/*TODO*/
	exit(1);
	return in;
}

/*Handle an entire expression.*/
static void handle_expression(char* expr){
	vstack_depth = 0;

	vstack_depth = 0;
}

static int find_and_handle_expressions(char* wkbuf,char* line_copy){
	
	long loc;
	long loc_end;
	long loc_vbar;
	char* expr;
	char* target;
	very_top:;
	loc = strfind(wkbuf, "${" /*}*/);
	loc_vbar = strfind(wkbuf, "|");
	if(loc == -1) return 0;

	/*respect the sequence point operator.*/
	if(loc_vbar != -1)
	if(loc_vbar < loc) return 0;

	/*we've found the last one! Now find where it ends...*/
	loc_end = strfind(wkbuf+loc+1,"}");
	if(loc_end == -1) goto error1;

	/*parse this expression*/

	expr = str_null_terminated_alloc(wkbuf+loc+2, loc_end);
	target = strcatalloc("","");
	if(!expr || !target) {fail_malloc_print();}

	PARSE_LEVEL_ASGN(expr, &target);

	/*
	my_strcpy(wkbuf+loc, wkbuf+loc+loc_end+1);
	*/
	if(strlen(target) >= WKBUF_SZ){
		puts("Expression compilation too large to fit on line!");
		exit(1);
	}
	memcpy(buf3, wkbuf, loc); buf3[loc] = '\0';
	if(loc + strlen(target) >= WKBUF_SZ){
		puts("Expression compilation slightly too large to fit on line!");
		exit(1);
	}
	strcat(buf3, target);
	if(strlen(buf3) + strlen(wkbuf+loc+loc_end+1) >= WKBUF_SZ){
		puts("Portion after expression would overflow work buffer.");
		exit(1);
	}
	strcat(buf3, wkbuf+loc+loc_end+1);
	my_strcpy(wkbuf, buf3);
	/*TODO: handle expr*/
	

	free(expr);
	free(target);
	if(strfind(wkbuf, "${" /*}*/) != -1) goto very_top;
	
	return 1;

	error1:;

	printf("\n<EXPRESSION PARSING ERROR> identified expression, but unable to find ending curly brace to expression.\nline: %s\nWkbuf: %s\n", line_copy, wkbuf);
	exit(1);
}
