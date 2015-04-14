int isWin( unsigned int score_left, unsigned int score_right){
  //Function to decide if one of the players has won the game.
  //The rule for winning is as follows
  //If either player has a score of 11, and the other player has a score of <10,
  //this player with score 11 wins the game.
  //If both players have a score of 10 or more, then if either player has a score
  //which is 2 points greater than the other, they win the game.
  //Return 0 if neither player has won, 1 if left side has won, 2 if right side has won
  int decision=0;
  if (score_left<11 && score_right<11){
	decision=0;
  }
  if(score_left==11 && score_right<10){
	decision=1;
  }
  if(score_right==11 && score_left<10){
    decision=2;
  }
  if (score_left>10 && score_right>10){
    if((score_left-2)==score_right){
	  decision=1;
	}
	else if((score_right-2)==score_left){
	  decision=2;
	}
	else{
	  decision=0;
	}
  }
  return decision;
}