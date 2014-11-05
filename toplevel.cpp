/*
 *   FILE: toplevel.c
 * AUTHOR: Muaaz Saleem (11besemsaleem@seecs.edu.pk)
 *   DATE: March 31 23:59:59 PST 2013
 *  DESCR:
 */

/* #define DEBUG */

#include "main.h"
#include <string>
#include "mazewar.h"

static bool		updateView;	/* true if update needed */
static bool		updateMissile;
MazewarInstance::Ptr M;
int Missile::missileCount = 0;
list<Missile> Missile::inflights;
int Packet::packet_count = 0;
list<Packet> Packet::packets_to_send;
list<RatRat> RatRat::all_the_rats;
int RatRat::rat_count = 0;
int RatRat::my_id;


/* Use this socket address to send packets to the multi-cast group. */
static Sockaddr         groupAddr;
#define MAX_OTHER_RATS  (MAX_RATS - 1)


int main(int argc, char *argv[])
{
    Loc x(1);
    Loc y(5);
    Direction dir(0);

    char *ratName;

    signal(SIGHUP, quit);
    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    getName("Welcome to CS244B MazeWar!\n\nYour Name", &ratName);
    ratName[strlen(ratName)-1] = 0;

    M = MazewarInstance::mazewarInstanceNew(string(ratName));
    MazewarInstance* a = M.ptr();
    strncpy(M->myName_, ratName, NAMESIZE);
    free(ratName);

    MazeInit(argc, argv);

    NewPosition(M);

	int my_id = getpid()%8-1;
	cout<<"\nId: "<<my_id<<endl;
	//M->myRatIdIs(my_id);
   // sendPacketToPlayers(RatId(1));

    //-----------------

    //-----------------
    /* So you can see what a Rat is supposed to look like, we create
    one rat in the single player mode Mazewar.
    It doesn't move, you can't shoot it, you can just walk around it */

    play();

    return 0;
}


/* ----------------------------------------------------------------------- */

void
play(void)
{
	MWEvent		event;
	Packet	incoming;

	event.eventDetail = &incoming;

	while (TRUE) {
		NextEvent(&event, M->theSocket());
		if (!M->peeking())
			switch(event.eventType) {
			case EVENT_A:
				aboutFace();
				break;

			case EVENT_S:
				leftTurn();
				Packet::create_packet('p');
				break;

			case EVENT_D:
				forward();
				Packet::create_packet('p');
				break;

			case EVENT_F:
				rightTurn();
				Packet::create_packet('p');
				break;


			case EVENT_LEFT_D:
				peekLeft();
				break;

			case EVENT_BAR:
				shoot();
				break;

			case EVENT_RIGHT_D:
				peekRight();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;
                       
                        case EVENT_TIMEOUT:
                                //do things that need to be done periodically
                               	manageMissiles();
                               	if(Packet::packets_to_send.size()>0) sendPacketToPlayers();
/*
                               	//UpdateScoreCard(0);
                               for (list<RatRat>::iterator it = RatRat::all_the_rats.begin(); it != RatRat::all_the_rats.end(); ++it)
		    						UpdateScoreCard(it->id);
		    					*/
                               	break; 

			case EVENT_INT:
				quit(0);
				break;

			}
		else
			switch (event.eventType) {
			case EVENT_RIGHT_U:
			case EVENT_LEFT_U:
				peekStop();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;
			}

		ratStates();		/* clean house */

		

		DoViewUpdate();

		/* Any info to send over network? */

	}
}


/* ----------------------------------------------------------------------- */

static	Direction	_aboutFace[NDIRECTION] ={SOUTH, NORTH, WEST, EAST};
static	Direction	_leftTurn[NDIRECTION] =	{WEST, EAST, NORTH, SOUTH};
static	Direction	_rightTurn[NDIRECTION] ={EAST, WEST, SOUTH, NORTH};

void
aboutFace(void)
{
	M->dirIs(_aboutFace[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
leftTurn(void)
{
	M->dirIs(_leftTurn[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
rightTurn(void)
{
	M->dirIs(_rightTurn[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

/* remember ... "North" is to the right ... positive X motion */

void
forward(void)
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case SOUTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case EAST:	if (!M->maze_[tx][ty+1])	ty++; break;
	case WEST:	if (!M->maze_[tx][ty-1])	ty--; break;
	default:
		MWError("bad direction in Forward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}
	
	//sendPacketToPlayers(RatId(0));
}

/* ----------------------------------------------------------------------- */

void backward()
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case SOUTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case EAST:	if (!M->maze_[tx][ty-1])	ty--; break;
	case WEST:	if (!M->maze_[tx][ty+1])	ty++; break;
	default:
		MWError("bad direction in Backward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekLeft()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(WEST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(EAST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(NORTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	default:
			MWError("bad direction in PeekLeft");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekRight()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(EAST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(WEST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(NORTH);
			}
			break;

	default:
			MWError("bad direction in PeekRight");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekStop()
{
	M->peekingIs(FALSE);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

int* Missile::nextMissileXY(int ox, int oy, int dir){
	cout<<"nextMissileXY called with\nox: "<<ox;
	cout<<"oy: "<<oy<<endl;	
	switch(dir) {
		case NORTH:	if (!M->maze_[ox+1][oy])	ox++; break;
		case SOUTH:	if (!M->maze_[ox-1][oy])	ox--; break;
		case EAST:	if (!M->maze_[ox][oy+1])	oy++; break;
		case WEST:	if (!M->maze_[ox][oy-1])	oy--; break;
		default:
		MWError("bad direction in nextMissileXY");
		}
	cout<<"ox: "<<ox;
	cout<<"oy: "<<oy<<endl;	
	int txy[2] = {ox,oy};
	cout<<" tx: "<<txy[0];
	cout<<" ty: "<<txy[1]<<endl;
	ox++;
	cout<<" tx: "<<txy[0];
	cout<<" ty: "<<txy[1]<<endl;
	return txy;
}



bool Missile::show(){
	cout<<"Missile::shown called"<<endl;
	cout<<"x: "<<this->x<<" y: "<<this->y<<endl;
	
	int *nxy = nextMissileXY(x, y, dir);
	//cout<<"*nx: "<<*(nxy+0)<<" *ny: "<<*(nxy+1)<<endl;
	int txy[2] = {nxy[0],nxy[1]};
		
	//this->x = *(nxy+0); this->y = *(nxy+1);
	//cout<<"x by *nx: "<<this->x<<" y by *ny: "<<this->y<<endl;
	if((txy[0]!= x || txy[1]!= y)){
		showMissile(txy[0], txy[1], dir, x, y, true);
		this->x = txy[0]; this->y = txy[1];

		cout<<"Missile shown calling updateView"<<endl;
		cout<<"new x:"<<this->x<<" new y:"<<this->y<<endl;
		cout<<"nx:"<<txy[0]<<" ny:"<<txy[1]<<endl;
		
  		list<Missile>::iterator it;
  		it = Missile::inflights.begin();
		for (int i=1; i<=Missile::missileCount; ++i){
    		Missile missile = *it;
    		if(missile.id==this->id){
    			Missile::inflights.erase(it);
    			Missile::inflights.push_back(*this);
    			updateView = TRUE;
    			++it;
    		}
		}
		updateView = TRUE;
		return true;

	}
	else {
		this->inflight = 0;
		Missile::missileCount--;
		clearSquare(txy[0], txy[1]);
		cout<<"Missile deleted calling updateView"<<endl;
		
		updateView = TRUE;
			return false;
	}
	
}

void shoot()
{	
	M->scoreIs( M->score().value()-1 );
	UpdateScoreCard(M->myRatId().value());
	int x = MY_X_LOC;
	int y = MY_Y_LOC;
	int *nxy = Missile::nextMissileXY(x, y, MY_DIR);
	int txy[2] = {*(nxy+0),*(nxy+1)};
		
	if ((x != txy[0]) || (y != txy[1])) {
		showMissile(txy[0], txy[1], MY_DIR, x, y, true);
		Missile missile = Missile(Missile::missileCount+1,1,txy[0],txy[1],MY_DIR);
		Missile::inflights.push_back(missile);
		updateView = TRUE;
		cout<<"I am shoot x:"<<x<<" y:"<<y<<" nx:";
		cout<<txy[0]<<" ny:"<<txy[1]<<endl;
		updateView = TRUE;



	}
}

/* ----------------------------------------------------------------------- */

/*
 * Exit from game, clean up window
 */

void quit(int sig)
{

	StopWindow();
	exit(0);
}


/* ----------------------------------------------------------------------- */

void NewPosition(MazewarInstance::Ptr m)
{
	Loc newX(0);
	Loc newY(0);
	Direction dir(0); /* start on occupied square */

	while (M->maze_[newX.value()][newY.value()]) {
	  /* MAZE[XY]MAX is a power of 2 */
	  newX = Loc(random() & (MAZEXMAX - 1));
	  newY = Loc(random() & (MAZEYMAX - 1));

	  /* In real game, also check that square is
	     unoccupied by another rat */
	}

	/* prevent a blank wall at first glimpse */

	if (!m->maze_[(newX.value())+1][(newY.value())]) dir = Direction(NORTH);
	if (!m->maze_[(newX.value())-1][(newY.value())]) dir = Direction(SOUTH);
	if (!m->maze_[(newX.value())][(newY.value())+1]) dir = Direction(EAST);
	if (!m->maze_[(newX.value())][(newY.value())-1]) dir = Direction(WEST);

	m->xlocIs(newX);
	m->ylocIs(newY);
	m->dirIs(dir);
}

/* ----------------------------------------------------------------------- */

void MWError(char *s)

{
	StopWindow();
	fprintf(stderr, "CS244BMazeWar: %s\n", s);
	perror("CS244BMazeWar");
	exit(-1);
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
Score GetRatScore(RatIndexType ratId)
{
  /*if (ratId.value() == 	M->myRatId().value())
    { return(M->score()); }
  else { return (0); }*/
    if (RatRat::match_in_list(ratId.value()))
  {
  	RatRat rat = RatRat::getRat(ratId.value());
  	return rat.score;
  }
  
  return M->score();
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
char *GetRatName(RatIndexType ratId)
{
  if (ratId.value() ==	M->myRatId().value())
    return(M->myName_);
  else if (RatRat::match_in_list(ratId.value())) {
  	RatRat rat = RatRat::getRat(ratId.value());
  	return rat.name;
  }

  return ("Dummy");
}

/*char *GetRatName(int rat_id)
{
  if (RatRat::match_in_list(rat_id))
  {
  	RatRat rat = RatRat::getRat(rat_id);
  	return rat.name;
  }
  else{
  	return "Dummy";
  }
}*/

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own if necessary */
void ConvertIncoming(Packet *p)
{
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own if necessary */
void ConvertOutgoing(Packet *p)
{
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void ratStates()
{
  /* In our sample version, we don't know about the state of any rats over
     the net, so this is a no-op */
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void manageMissiles()
{
  /* Leave this up to you. */
	
  	list<Missile>::iterator it;
  	it = Missile::inflights.begin();
	for (int i=1; i<=Missile::missileCount; ++i){
    	Missile missile = *it;
    		cout<<"manageMISSILE called: "<<i<<endl;
    		cout<<"id: "<<missile.id<<endl;
    		cout<<"inflight: "<<missile.inflight<<endl;
    		cout<<"x: "<<missile.x<<endl;
    		cout<<"y: "<<missile.y<<endl;
    		
    		
    		if((!missile.show()) || (missile.rat_id != RatRat::my_id)){
    			Missile::inflights.erase(it);
    			updateView = TRUE;
    			++it;
    		}
    	missile.create_packet();
	}
}

/* ----------------------------------------------------------------------- */

void DoViewUpdate()
{
	if (updateView) {	/* paint the screen */
		ShowPosition(MY_X_LOC, MY_Y_LOC, MY_DIR);
		if (M->peeking())
			ShowView(M->xPeek(), M->yPeek(), M->dirPeek());
		else
			ShowView(MY_X_LOC, MY_Y_LOC, MY_DIR);
		updateView = FALSE;
	}
}

/* ----------------------------------------------------------------------- */

/*
 * Sample code to send a packet to a specific destination
 */

/*
 * Notice the call to ConvertOutgoing.  You might want to call ConvertOutgoing
 * before any call to sendto.
 */

bool Packet::create_packet(unsigned char type){
	cout<<"Create Packet Called>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
	Packet packet;
	switch(type){
		case 'i':
				packet.type = 'i';
				packet.body[0] = RatRat::my_id;
				cout<<"Name Array: ";
				for (int j = 0; j < NAMESIZE; ++j)
				{
					packet.body[1+j]=M->myName_[j];
					cout<<(char)packet.body[1+j];
				}
				cout<<endl;
				cout<<"First Character: "<<(char)packet.body[1]<<endl;
				cout<<"Last Character: "<<(char)packet.body[NAMESIZE-1]<<endl;
				break;
		case 'p':
				packet.type = 'p';
				packet.body[0] = RatRat::my_id;
				packet.body[1] = MY_X_LOC;
				packet.body[2] = MY_Y_LOC;
				packet.body[2] = MY_DIR;
				cout<<"Making a packet type: "<<packet.type<<endl;
				break;
		case 's':
				packet.type = 's';
				packet.body[0] = RatRat::my_id;
				packet.body[1] = (int)M->score().value();
				break;


		  //.... set other fields in the packet  that you need to set...
	}
	packet.add_to_list();
	return true;
}

void sendPacketToPlayers()
{
/*
	Packet pack;
	DataStructureX *packX;

	pack.type = PACKET_TYPE_X;
	packX = (DataStructureX *) &pack.body;
	packX->foo = d1;
	packX->bar = d2;

        ....

	ConvertOutgoing(pack);

	if (sendto((int)mySocket, &pack, sizeof(pack), 0,
		   (Sockaddr) destSocket, sizeof(Sockaddr)) < 0)
	  { MWError("Sample error") };
*/
	list<Packet>::iterator it;
  	it = Packet::packets_to_send.begin();

	for (int i=1; i<=Packet::packet_count; ++i){
    	Packet pack = *it;
    	cout<<"sendPacketToPlayers called: "<<i<<endl;
    	cout<<"Packet Count: "<<Packet::packet_count<<endl;
    	cout<<"type: "<<pack.type<<endl;
    	cout<<"id: "<<pack.body[0]<<endl;
    	cout<<"name: "<<(char)pack.body[1]<<endl;
    	ConvertOutgoing(&pack);

        if (sendto((int)M->theSocket(), &pack, sizeof(pack), 0,
                    (const sockaddr*)&groupAddr, sizeof(Sockaddr)) < 0)
          { MWError("Sample error");}
      	
      	//Packet::packets_to_send.erase(it);
      	//Packet::packet_count--;
      	
    		
    	++it;
    }
	

	 
      

       
 
}

/* ----------------------------------------------------------------------- */

/* Sample of processPacket. */

void processPacket (MWEvent *eventPacket)
{
/*
	Packet		*pack = eventPacket->eventDetail;
	DataStructureX		*packX;

	switch(pack->type) {
	case PACKET_TYPE_X:
	  packX = (DataStructureX *) &(pack->body);
	  break;
        case ...
	}
*/
	Packet *pack = eventPacket->eventDetail;
    //cout<<"type: "<<pack->type<<" received"<<endl;
    int sender_id = pack->body[0];
    	if(RatRat::my_id != sender_id){
		    switch(pack->type){
		    	case 'i':
		    		
		    		if(!RatRat::match_in_list(sender_id)){
		    			cout<<"id: "<<sender_id<<" received"<<endl;
		    			char sender_name[2];
		    			cout<<"name: ";
		    			for (int j = 0; j < 2; ++j)
				    	{	
				    		sender_name[j] = (char)pack->body[1+j];
				    		cout<<sender_name[j];
				    	}
				    	cout<<" received"<<endl;

				    	RatRat newRat(sender_id, sender_name);//,sender_name);
				    	cout<<"newRat made\n";
				    	
			    		newRat.add_to_list();
			    		UpdateScoreCard(sender_id);
			    		NotifyPlayer();
				    	
				    	updateView = TRUE;
		    		}
		    		break;
		    	case 'p':
		    		
		    		if(RatRat::match_in_list(sender_id)){
		    			cout<<">>>>>>>>Packet of type"<<pack->type<<" received<<<<<<<<<<<<<<\n";
		    			int sender_xloc = (int)pack->body[1];
		    			int sender_yloc = (int)pack->body[2];
		    			int sender_dir = (int)pack->body[3];
		    			RatRat this_rat = RatRat::getRat(sender_id);
		    			this_rat.x = sender_xloc;
		    			this_rat.y = sender_yloc;
		    			this_rat.dir = sender_dir;
		    			this_rat.add_to_list();
		    			SetRatPosition(RatIndexType(0), this_rat.x, this_rat.y, this_rat.dir);
		    		}
		    		break;
		    	case 'm':
		    		if(RatRat::match_in_list(sender_id)){
			    		cout<<"<><><><><><<>>Receiving packet OF type: "<<pack->type<<endl;
						int sender_id = (int)pack->body[0];
						int sender_xloc = (int)pack->body[1];
			    		int sender_yloc = (int)pack->body[2];
			    		int sender_dir = (int)pack->body[3];
			    		Missile missile = Missile(Missile::missileCount+1,1,sender_xloc,sender_yloc,sender_id);
		    			missile.rat_id = sender_id;
						Missile::inflights.push_back(missile);
			    	}
					break;
				case 's':
					if(RatRat::match_in_list(sender_id)){
						int sender_id = (int)pack->body[0];
						int sender_score  = (int)pack->body[1];
						RatRat this_rat = RatRat::getRat(sender_id);
						this_rat.score = sender_score;
						this_rat.add_to_list();
						UpdateScoreCard(sender_id);

					}
					break;
	    	}
	    }
 
        /*DataStructureX                *packX;

        switch(pack->type) {
        case PACKET_TYPE_X:
          packX = (DataStructureX *) &(pack->body);
          break;
        case ...
        }
        */

}

/* ----------------------------------------------------------------------- */

/* This will presumably be modified by you.
   It is here to provide an example of how to open a UDP port.
   You might choose to use a different strategy
 */
void
netInit()
{
	Sockaddr		nullAddr;
	Sockaddr		*thisHost;
	char			buf[128];
	int				reuse;
	u_char          ttl;
	struct ip_mreq  mreq;

	/* MAZEPORT will be assigned by the TA to each team */
	M->mazePortIs(htons(MAZEPORT));

	gethostname(buf, sizeof(buf));
	if ((thisHost = resolveHost(buf)) == (Sockaddr *) NULL)
	  MWError("who am I?");
	bcopy((caddr_t) thisHost, (caddr_t) (M->myAddr()), sizeof(Sockaddr));

	M->theSocketIs(socket(AF_INET, SOCK_DGRAM, 0));
	if (M->theSocket() < 0)
	  MWError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(M->theSocket(), SOL_SOCKET, SO_REUSEADDR, &reuse,
		   sizeof(reuse)) < 0) {
		MWError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = M->mazePort();
	if (bind(M->theSocket(), (struct sockaddr *)&nullAddr,
		 sizeof(nullAddr)) < 0)
	  MWError("netInit binding");

	/* Multicast TTL:
	   0 restricted to the same host
	   1 restricted to the same subnet
	   32 restricted to the same site
	   64 restricted to the same region
	   128 restricted to the same continent
	   255 unrestricted

	   DO NOT use a value > 32. If possible, use a value of 1 when
	   testing.
	*/

	ttl = 1;
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
		   sizeof(ttl)) < 0) {
		MWError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(MAZEGROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)
		   &mreq, sizeof(mreq)) < 0) {
		MWError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/*
	 * Now we can try to find a game to join; if none, start one.
	 */
	 
	
	/* set up some stuff strictly for this local sample */
	
	M->myRatIdIs(0);
	M->scoreIs(0);
	SetMyRatIndexType(0);
	

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&groupAddr, &nullAddr, sizeof(Sockaddr));
	groupAddr.sin_addr.s_addr = htonl(MAZEGROUP);
	

	RatRat::my_id = getpid()%7+1; 
	Packet::create_packet('i');
	


}




/* ----------------------------------------------------------------------- */
