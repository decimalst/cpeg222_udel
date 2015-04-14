int whoServe(unsigned int score_left, unsigned int score_right){
	//Function to determine who serves the ball next.
	//Returns, according to the services rules, who serves.
	//Returns 0x80 if the left player should serve, and
	//returns 0x1 if the right player should serve, which correspond to
	//the pmod8LED status for each serve case.
	//The left player always serves for the first 2 turns, then the right
	//player for two turns, until the players both have ten points or more,
	//then each player serves the ball for only one point in their turn.
	int sum = score_right+score_left;
	int to_serve;
	if((sum)<20){
		if(((sum/2)%2)==0){
			//then left player serves
			//On turn 1,2, sum=0,1 respectively, while left serves,
			//and ((sum/2)%2)==0.
			//On turn 3,4, sum=2,3 respectively, while right serves,
			//and ((sum/2)%2)==1
			//On turn 5,6, sum=4,5 respectively, while left serves,
			//and ((sum/2)%2)== 0.
			//Thus, when ((sum/2)%2)==0, left serves and vice versa.
			to_serve=0x80;
		}
		else{
			//then right player serves
			to_serve=0x1;
		}
	}
	if((sum)>=20){
		if(sum%2==0){
			to_serve=0x80;
		}
		else{
			to_serve=0x1;
		}
	}
	return to_serve;
}