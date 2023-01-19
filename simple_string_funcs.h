

static char my_isoct(char c){
	if(c == '0') return 1;
	if(c == '1') return 1;
	if(c == '2') return 1;
	if(c == '3') return 1;
	if(c == '4') return 1;
	if(c == '5') return 1;
	if(c == '6') return 1;
	if(c == '7') return 1;
	return 0;
}

static char my_isdigit(char c){
	if(c == '0') return 1;
	if(c == '1') return 1;
	if(c == '2') return 1;
	if(c == '3') return 1;
	if(c == '4') return 1;
	if(c == '5') return 1;
	if(c == '6') return 1;
	if(c == '7') return 1;
	if(c == '8') return 1;
	if(c == '9') return 1;
	return 0;
}

static char my_ishex(char c){
	if(c == '0') return 1;
	if(c == '1') return 1;
	if(c == '2') return 1;
	if(c == '3') return 1;
	if(c == '4') return 1;
	if(c == '5') return 1;
	if(c == '6') return 1;
	if(c == '7') return 1;
	if(c == '8') return 1;
	if(c == '9') return 1;
	if(c == 'A') return 1;
	if(c == 'a') return 1;
	if(c == 'B') return 1;
	if(c == 'b') return 1;
	if(c == 'C') return 1;
	if(c == 'c') return 1;
	if(c == 'D') return 1;
	if(c == 'd') return 1;
	if(c == 'E') return 1;
	if(c == 'e') return 1;
	if(c == 'F') return 1;
	if(c == 'f') return 1;
	return 0;
}

