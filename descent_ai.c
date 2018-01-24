#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <dirent.h>

#ifdef WINDOWS
#include <windows.h>
#endif

#define MAX_CARD_COST 25
#define PRINT_WIDTH 80

//chances represent the number of threat the OL must have above
//the card cost to guarantee playing the card.  Lowering the 
//chance increases the likelihood the card type is played.
#define TRAP_DOOR_CHANCE 25
#define TRAP_CHEST_CHANCE 25
#define TRAP_SPACE_CHANCE 25
#define TRAP_CHANCE 25
#define EVENT_CHANCE 25
#define SPAWN_CHANCE 25
#define POWER_CHANCE 8

FILE * quest_file;

typedef struct{
    int players;
    int quest;
    int room[10];
    char questName[100];
    int countdown;
    char cdtext[1000];
} gm;
gm game;

typedef struct{
    int threat;
    int conquest_tokens;
} ol;
ol overlord = {0};

typedef enum {
    event, spawn, power, trap_door, trap_space, trap_chest, trap
} ol_card_type;

typedef struct{
    int play_cost;
    int sell_cost;
    ol_card_type type;
    char* name;
    char* text;
} card;
card deck[60];
int deckSize = 36;
card fulldeck[60];
int fullSize = 0;
card hand[15];
int handSize = 0;
card discard[60];
int discardSize = 0;
card table[20];
int tableSize = 0;

typedef struct{
    int movement;
} monster;
monster monsters[60];
int monsterSize = 0;

#ifdef WINDOWS
size_t getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}

int scandirWin (char *fullPath, char *folderName, int level, char *fileNames[]){
	
    char findpath[_MAX_PATH], temppath[_MAX_PATH];
    HANDLE fh;
    WIN32_FIND_DATA wfd;
    int i;
	
	char *holder;
	int fileNum = 2;

    strcpy (findpath, fullPath);
    if (folderName)
    {
            strcat (findpath, "\\");
            strcat (findpath, folderName);
    }
    strcat (findpath, "\\*.*");

    fh = FindFirstFile (findpath, &wfd);
    if (fh != INVALID_HANDLE_VALUE)
    {
            do
            {
                    if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    {
							holder = malloc(128);
							strcpy(holder, wfd.cFileName);
							fileNames[fileNum] = holder;
							fileNum++;
                    }
            } while (FindNextFile (fh, &wfd));
            FindClose (fh);
    }
	return fileNum;
}
#endif

void printCards(card* ptr, int size){
    int i;
    for(i = 0; i<size; i++) printf("%2d: Cost: %d \tSell: %d \t%s\n", i, ptr[i].play_cost, ptr[i].sell_cost, ptr[i].name);
    printf("\n");
}

void discardCard(int count, int low, int high){
    int temp = rand()%count;
    int i;
    for(i = 0; i<handSize; i++){
        if(low <= hand[i].play_cost &&  hand[i].play_cost <= high){
            if(temp == 0){
                discard[discardSize] = hand[i];
                overlord.threat += hand[i].sell_cost;
                discardSize++;
                int j;
                for(j = i; j<handSize; j++)
                    hand[j] = hand[j+1];
                handSize--;
                return;
            }
            temp--;
        }	
    }
}

void draw_card(){
    //get card
    int cnum = rand()%deckSize;
    hand[handSize] = deck[cnum];
    handSize++;
    
    int i;
    //printCards(hand, handSize);
    
    for(i = cnum; i<deckSize; i++){
        deck[i] = deck[i+1];
    }
    deckSize--;
    
    //optional discard
    
    //if more than 8 discard for threat
    while(handSize > 8){
        int low = 0, med = 0, high = 0;
        for(i = 0; i<handSize; i++){
            if(hand[i].play_cost<5)
                low++;
            else if(hand[i].play_cost<15)
                med++;
            else
                high++;
        }
        //if more than one high cost card in hand pick high cost cards to discard
        if(high > 1){
            discardCard(high, 15, 30);
        }
        else if(med > 3){
            discardCard(med, 5, 14);
        }
        else if(low > 4){
            discardCard(low, 1, 4);
        }
        //printCards(hand, handSize);
    }
    
}

void keyPause(){
    printf("\n<><>Press ENTER to Continue<><>");
    getchar();
    printf("\n\n");
}

void init_ol(){
    overlord.threat = 0;
    //overlord.conquest_tokens = 4; //varies by quest, players can track conquest tokens
    draw_card();
    draw_card();
    draw_card();
}

void init_deck(){

    int i;
    for(i = 0; i<=2; i++){
        deck[i].play_cost = 2;
        deck[i].sell_cost = 1;
        deck[i].type = event;
        deck[i].name = strdup("Aim");
        deck[i].text = strdup("Play immediately after declaring an attack "\
                              "but before rolling any dice. Your attack is "\
                              "an aimed attack.");
    }
    for(i = 3; i<=4; i++){
        deck[i].play_cost = 4;
        deck[i].sell_cost = 2;
        deck[i].type = spawn;
        deck[i].name = strdup("Bane Spider Swarm");
        deck[i].text = strdup("Play this card before activating any of your "\
                              "monsters for the turn. You may spawn 2 Bane "\
                              "Spiders and 1 Master Bane Spider, following all "\
                              "of the normal rules for spawning monsters.");
    }
    for(i = 5; i<=6; i++){
        deck[i].play_cost = 4;
        deck[i].sell_cost = 2;
        deck[i].type = spawn;
        deck[i].name = strdup("Beastman War Party");
        deck[i].text = strdup("Play this card before activating any of your "\
                              "monsters for the turn. You may spawn 2 Beastmen "\
                              "and 1 Master Beastman, following all of the "\
                              "normal rules for spawning monsters.");
    }
    deck[7].play_cost = 17;
    deck[7].sell_cost = 5;
    deck[7].type = power;
    deck[7].name = strdup("Brilliant Commander");
    deck[7].text = strdup("Play this card at the start of your turn and "\
                          "place it face up in front of you. When you reveal "\
                          "a new area, upgrade any 1 monster in that area to "\
                          "a master of that monster's type.");
    for(i = 8; i<=9; i++){
        deck[i].play_cost = 4;
        deck[i].sell_cost = 1;
        deck[i].type = event;
        deck[i].name = strdup("Charge");
        deck[i].text = strdup("Play when you activate a monster. Double "\
                              "the monster's speed for this activation.");
    }
    deck[10].play_cost = 7;
    deck[10].sell_cost = 3;
    deck[10].type = trap_space;
    deck[10].name = strdup("Crushing Block");
    deck[10].text = strdup("Play this card when a hero moves into an empty "\
                           "space that is not adjacent to any obstacles. Place "\
                           "a one space rubble token on that space. The hero "\
                           "suffers 4 wounds (ignoring armor), reduced by 1 wound "\
                           "for each surge he rolls on 4 power dice. Then, move "\
                           "the hero to an adjacent space of your choice.");
    deck[11].play_cost = 11;
    deck[11].sell_cost = 4;
    deck[11].type = trap_chest;
    deck[11].name = strdup("Curse of the Monkey God");
    deck[11].text = strdup("Play this card when a hero opens a chest. The hero "\
                           "player must roll a power die. If he rolls blank, "\
                           "nothing happens. If he does not roll a blank, the "\
                           "hero is transformed into a monkey.");
    deck[12].play_cost = 8;
    deck[12].sell_cost = 2;
    deck[12].type = trap;
    deck[12].name = strdup("Dark Charm");
    deck[12].text = strdup("Play at the start of your turn and choose one hero. "\
                           "The chosen hero player must roll one power die. If "\
                           "the result is blank, nothing happens. If the result "\
                           "is not a blank, the hero must make one attack that you "\
                           "declare. This attack may target any hero, including the "\
                           "attacking hero, but is subject to the normal attack rules, "\
                           "including range and line of sight.");
    for(i = 13; i<=15; i++){
        deck[i].play_cost = 2;
        deck[i].sell_cost = 1;
        deck[i].type = event;
        deck[i].name = strdup("Dodge");
        deck[i].text = strdup("Play after a hero has attacked a monster. "\
                              "The monster dodges the attack.");
    }
    deck[16].play_cost = 20;
    deck[16].sell_cost = 5;
    deck[16].type = power;
    deck[16].name = strdup("DOOM!");
    deck[16].text = strdup("Play this card at the start of your turn and "\
                           "place it face up in front of you. All of your "\
                           "monsters roll 1 extra power die when attacking.");
    deck[17].play_cost = 25;
    deck[17].sell_cost = 5;
    deck[17].type = power;
    deck[17].name = strdup("Evil Genius");
    deck[17].text = strdup("Play this card at the start of your turn and place "\
                           "it face up in front of you. Draw 1 extra overlord "\
                           "card each turn.");
    deck[18].play_cost = 9;
    deck[18].sell_cost = 3;
    deck[18].type = trap_door;
    deck[18].name = strdup("Explosive Door");
    deck[18].text = strdup("Play this card when a hero opens a door. Every hero "\
                           "adjacent to the door suffers 4 wounds (ignoring armor). "\
                           "Each affected hero may roll 4 power dice and reduce the "\
                           "effect by 1 wound for each surge he rolls.");
    deck[19].play_cost = 12;
    deck[19].sell_cost = 4;
    deck[19].type = trap_chest;
    deck[19].name = strdup("Explosive Rune");
    deck[19].text = strdup("Play this card when a hero opens a chest. The hero "\
                           "that opened the chest, as well as every hero "\
                           "adjacent to the chest, suffers 6 wounds (ignoring armor). "\
                           "Each affected hero may roll 3 power dice and reduce the "\
                           "effect by 2 wounds for each surge he rolls.");
    for(i = 20; i<=21; i++){
        deck[i].play_cost = 6;
        deck[i].sell_cost = 2;
        deck[i].type = event;
        deck[i].name = strdup("Gust of Winds");
        deck[i].text = strdup("Play at the start of your turn. Until the start "\
                              "of your next turn, the heroes' torches are blown "\
                              "out and they cannot trace line of sight farther "\
                              "than 5 spaces away. This card does not affect "\
                              "monsters' line of sight.");
    }
    deck[22].play_cost = 5;
    deck[22].sell_cost = 2;
    deck[22].type = spawn;
    deck[22].name = strdup("Hell Hound Pack");
    deck[22].text = strdup("Play this card before activating any of your "\
                           "monsters for the turn. You may spawn 2 Hell "\
                           "Hounds, following all of the normal rules for "\
                           "spawning monsters.");
    for(i = 23; i<=24; i++){
        deck[i].play_cost = 18;
        deck[i].sell_cost = 5;
        deck[i].type = power;
        deck[i].name = strdup("Hordes of the Things");
        deck[i].text = strdup("Play this card at the start of your turn "\
                              "and place it face up in front of you. When "\
                              "you reveal a new area, place your choice of: "\
                              "2 extra Beastmen, 2 extra Skeletons, or 2 extra "\
                              "Bane Spiders adjacent to any other monster in "\
                              "the revealed area.");
    }
    deck[25].play_cost = 8;
    deck[25].sell_cost = 3;
    deck[25].type = trap_chest;
    deck[25].name = strdup("Mimic");
    deck[25].text = strdup("Pay this card when a hero opens a chest. The chest "\
                           "is alive, and its contents cannot be distributed "\
                           "until it is killed. Move the chest marker to an "\
                           "adjacent space. Threat it as a Beastman and activate "\
                           "it immediately. After its activation, the hero's "\
                           "turn resumes. If the chest is killed, the contents "\
                           "of the chest are immediately distributed.");
    deck[26].play_cost = 5;
    deck[26].sell_cost = 3;
    deck[26].type = trap_door;
    deck[26].name = strdup("Paralyzing Gas");
    deck[26].text = strdup("Play this card when a hero opens a door. Unless the "\
                           "hero rolls a blank on one power die, his turn "\
                           "immediately ends and you may place one stun token on him. "\
                           "If the hero player rolls a blank, this card has no effect.");
    for(i = 27; i<=28; i++){
        deck[i].play_cost = 4;
        deck[i].sell_cost = 1;
        deck[i].type = event;
        deck[i].name = strdup("Rage");
        deck[i].text = strdup("Play when you activate a monster. The monster may "\
                              "attack twice during this activation (four times if "\
                              "it has Quick Shot).");
    }
    deck[29].play_cost = 5;
    deck[29].sell_cost = 2;
    deck[29].type = spawn;
    deck[29].name = strdup("Razorwing Flock");
    deck[29].text = strdup("Play this card before activating any of your monsters "\
                           "for the turn. You may spawn 2 Razorwings, following "\
                           "all of the normal rules for spawning monsters.");
    for(i = 30; i<=31; i++){
        deck[i].play_cost = 4;
        deck[i].sell_cost = 2;
        deck[i].type = spawn;
        deck[i].name = strdup("Skeleton Patrol");
        deck[i].text = strdup("Play this card before activating any of your monsters "\
                              "for the turn. You may spawn 2 Skeletons and 1 Master "\
                              "Skeleton, following all of the normal rules for "\
                              "spawning monsters.");
    }
    deck[32].play_cost = 5;
    deck[32].sell_cost = 2;
    deck[32].type = spawn;
    deck[32].name = strdup("Sorcerer Circle");
    deck[32].text = strdup("Play this card before activating any of your monsters "\
                           "for the turn. You may spawn 2 Sorcerers, following all "\
                           "of the normal rules for spawning monsters.");
    for(i = 33; i<=34; i++){
        deck[i].play_cost = 3;
        deck[i].sell_cost = 2;
        deck[i].type = trap_space;
        deck[i].name = strdup("Spiked Pit");
        deck[i].text = strdup("Play this card when a hero moves into an empty "\
                              "space. Place a one space pit token on that space. "\
                              "Unless the hero rolls a blank on one power die, he "\
                              "falls into the pit and suffers 2 wounds (ignoring "\
                              "armor). If the hero rolls a blank, the hero immediately "\
                              "moves to an adjacent space of his choice.");
    }
    deck[35].play_cost = 16;
    deck[35].sell_cost = 5;
    deck[35].type = power;
    deck[35].name = strdup("Trapmaster");
    deck[35].text = strdup("Play this card at the start of your turn and place "\
                           "it face up in front of you. Lower the threat cost of "\
                           "the Trap cards you play by 1. Trap cards that deal "\
                           "damage deal an additional 2 wounds.");
    
    deckSize = 36;
    
    fulldeck[fullSize] = deck[0]; fullSize++;
    fulldeck[fullSize] = deck[3]; fullSize++;
    fulldeck[fullSize] = deck[5]; fullSize++;
    fulldeck[fullSize] = deck[7]; fullSize++;
    fulldeck[fullSize] = deck[8]; fullSize++;
    fulldeck[fullSize] = deck[10]; fullSize++;
    fulldeck[fullSize] = deck[11]; fullSize++;
    fulldeck[fullSize] = deck[12]; fullSize++;
    fulldeck[fullSize] = deck[13]; fullSize++;
    fulldeck[fullSize] = deck[16]; fullSize++;
    fulldeck[fullSize] = deck[17]; fullSize++;
    fulldeck[fullSize] = deck[18]; fullSize++;
    fulldeck[fullSize] = deck[19]; fullSize++;
    fulldeck[fullSize] = deck[20]; fullSize++;
    fulldeck[fullSize] = deck[22]; fullSize++;
    fulldeck[fullSize] = deck[23]; fullSize++;
    fulldeck[fullSize] = deck[25]; fullSize++;
    fulldeck[fullSize] = deck[26]; fullSize++;
    fulldeck[fullSize] = deck[27]; fullSize++;
    fulldeck[fullSize] = deck[29]; fullSize++;
    fulldeck[fullSize] = deck[30]; fullSize++;
    fulldeck[fullSize] = deck[32]; fullSize++;
    fulldeck[fullSize] = deck[33]; fullSize++;
    fulldeck[fullSize] = deck[35]; fullSize++;
}

void resuffleDiscard(){
    int i;
    for(i = 0; i<discardSize; i++){
        deck[deckSize] = discard[i];
        deckSize++;
    }
    discardSize = 0;
    printf("The overlord has shuffled his deck of cards!\n Remove 3 Conquest Tokens!\n");
    keyPause();
}

void collect_threat(int players){
    overlord.threat += players;
}

void printSection(char * tag){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    quest_file = fopen(game.questName, "r");
    while ((read = getline(&line, &len, quest_file)) != -1) {
        if(strcmp(line, tag) == 0){
            getline(&line, &len, quest_file);
            while(line[0] != '#'){
                while(line[0] == '<'){
                    getline(&line, &len, quest_file);
                    getline(&line, &len, quest_file);
                }
                int startP = 0;
                int endP = 0;
                int tempP = 0;
                for(endP = 0; endP < strlen(line); endP ++){
                    if(line[endP] == ' '){
                        if(endP - startP > PRINT_WIDTH){
                            printf("%.*s\n", tempP-startP, line+startP);
                            startP = tempP + 1;
                        }
                        tempP = endP;
                    }
                    if(endP == strlen(line) - 1){
                        if(endP - startP > PRINT_WIDTH){
                            printf("%.*s\n", tempP-startP, line+startP);
                            startP = tempP + 1;
                        }
                        printf("%.*s", endP-startP+1, line+startP);
                    }
                }
                getline(&line, &len, quest_file);
            }
        }
    }
    fclose(quest_file);
    
}

void init_game(){

    printf("Number of players: ");
    scanf("%d%*c", &game.players);
    printf("Which Quest will you play?\n");
    
    int cnt = 1;
    struct dirent **namelist;
	char *fileNames[128];
    int j, n;
	
	#ifdef WINDOWS
	n = scandirWin("./Quests/", NULL, 0, fileNames);
	#else
    n = scandir("./Quests/", &namelist, 0, alphasort);
	#endif
    if (n < 0)
        perror("scandir");
    else {
        for (j = 2; j < n; j++) {
			#ifdef WINDOWS
            printf("%d: %s\n", cnt++, fileNames[j]);
			#else
            printf("%d: %s\n", cnt++, namelist[j]->d_name);
			#endif
        }
    
        printf("Selection: ");
        scanf("%d%*c", &game.quest);
        game.quest++;
    
        for (j = 2; j < n; j++) {
            if(j == game.quest){
                strcpy(game.questName, "./Quests/");
				#ifdef WINDOWS
                strcat(game.questName, fileNames[j]);
				#else
                strcat(game.questName, (namelist[j]->d_name));
				#endif
                printf("%s", game.questName);
            }
			#ifdef LINUX
            free(namelist[j]);
			#endif
        }
    }
	#ifdef LINUX
    free(namelist);
	#endif
      
    game.countdown = 0;
    
    init_deck();
    init_ol();
    
    game.room[0] = 1;
    int i;
    for(i=1; i<10; i++)
        game.room[i] = 0;
    printf("\nSCENARIO BACKGROUND\n\n");
    printSection("#SCENBACK:\n");
    printf("\nMISSION GOAL\n\n");
    printSection("#MISSGOAL:\n");
    printf("\nSTARTING AREA\n\n");
    printSection("#S:\n");
}

int inplay(char* card_name){
    int i;
    int count = 0;
    for(i = 0; i<tableSize; i++){
        if(strcmp(table[i].name, card_name) == 0){
            count++;
        }
    }
    return count;
}

void printCard(card c){
    printf(" ______________________________\n");
    printf("/                              \\\n");
    printf("|%*s%*s|\n",15+strlen(c.name)/2,c.name,15-strlen(c.name)/2,"");
    printf("|------------------------------|\n"); //30
    char* type;
    if(c.type == 0) type = "Event";
    else if(c.type == 1) type = "Spawn";
    else if(c.type == 2) type = "Power";
    else if(c.type == 3) type = "Trap (Door)";
    else if(c.type == 4) type = "Trap (Space)";
    else if(c.type == 5) type = "Trap (Chest)";
    else type = "Trap";
    printf("|%*s%*s|\n",15+strlen(type)/2,type,15-strlen(type)/2,"");
    printf("|                              |\n");
    int startP = 0;
    int endP = 0;
    int tempP = 0;
    for(endP = 0; endP < strlen(c.text); endP ++){
        if(c.text[endP] == ' '){
            if(endP - startP > 30){
                printf("|%*s%.*s%*s|\n", 15-(tempP-startP)/2, "", tempP-startP, c.text+startP, 30-(tempP-startP)-(15-(tempP-startP)/2), "");
                startP = tempP + 1;
            }
            tempP = endP;
        }
        if(endP == strlen(c.text) - 1){
            if(endP - startP > 30){
                printf("|%*s%.*s%*s|\n", 15-(tempP-startP)/2, "", tempP-startP, c.text+startP, 30-(tempP-startP)-(15-(tempP-startP)/2), "");
                startP = tempP + 1;
            }
            printf("|%*s%.*s%*s|\n", 15-(endP-startP+1)/2, "", endP-startP+1, c.text+startP, 30-(endP-startP+1)-(15-(endP-startP+1)/2), "");
        }
    }
    
    printf("| __                        __ |\n");
    printf("|/  \\                      /  \\|\n");
    printf("| %2d |                    | %2d |\n", c.play_cost, c.sell_cost);
    printf(" \\__/______________________\\__/\n");
	printf(" Cost                      Value\n");
    
    if(inplay("Trapmaster") > 0){
        if(c.type == trap_door || c.type == trap_space || c.type == trap_chest || c.type == trap){
            printf("Don't forget, %dx Trapmaster cards in play!\n", inplay("Trapmaster"));
        }
    }
    
    keyPause();
}

void playCard(int i){
    printf("\n^^The Overlord plays the following card:^^\n");
    printCard(hand[i]);
    
    if(inplay("Trapmaster") > 0){
        if(hand[i].type == trap_door || hand[i].type == trap_space || hand[i].type == trap_chest || hand[i].type == trap){
            overlord.threat += inplay("Trapmaster");
        }
    }
    
    if(hand[i].type == power){
        table[tableSize] = hand[i];
        overlord.threat -= hand[i].play_cost;
        tableSize++;
    }
    else{
        discard[discardSize] = hand[i];
        overlord.threat -= hand[i].play_cost;
        discardSize++;
    }
    int j;
    for(j = i; j<handSize; j++)
        hand[j] = hand[j+1];
    handSize--;
}

void spawn_monsters(int open_squares){
    int i;
    for(i = 0; i<handSize; i++){
        if(hand[i].type == spawn && overlord.threat >= hand[i].play_cost){
            int open_check = 0;
            if(!strcmp(hand[i].name, "Beastman War Party")) open_check = 3;
            else if(!strcmp(hand[i].name, "Bane Spider Swarm")) open_check = 12;
            else if(!strcmp(hand[i].name, "Hell Hound Pack")) open_check = 3;
            else if(!strcmp(hand[i].name, "Razorwing Flock")) open_check = 3;
            else if(!strcmp(hand[i].name, "Skeleton Patrol")) open_check = 3;
            else if(!strcmp(hand[i].name, "Sorcerer Circle")) open_check = 3;
            if(open_squares >= open_check){
                int extra_threat = overlord.threat - hand[i].play_cost;
                int temp = rand()%SPAWN_CHANCE;
                if(extra_threat >= temp){
                        playCard(i);
                        break;
                }
            }
        }
    }
}

void activate_monsters(){
    //print general AI for all monsters for this turn
    int ai = rand()%100;
    if(ai < 20) printf("Melee: First, monsters adjacent to heroes attack \n"\
                       "adjacent hero, then move as far away from all heroes\n"\
                       "as possible.  Mosters not next to a hero run toward \n"\
                       "closest hero and attack. If multiple heroes are in range \n"\
                       "attack the one with the least health.\n\n"\
                       "Ranged: If within range 6 of a hero, attack weakest hero \n"\
                       "then move as far from all heroes as possible.  If starting \n"\
                       "at range greater than 6, move as close as range 4 and attack \n"\
                       "the hero with the least life remaining.\n");
    else if(ai < 40) printf("Melee: First, monsters adjacent to heroes attack \n"\
                            "adjacent hero, then move as far away from all heroes\n"\
                            "as possible.  Mosters not next to a hero run toward \n"\
                            "closest hero and attack. If multiple heroes are in range \n"\
                            "attack the one with the most health.\n\n"\
                            "Ranged: If within range 6 of a hero, attack weakest hero \n"\
                            "then move as far from all heroes as possible.  If starting \n"\
                            "at range greater than 6, move as close as range 4 and attack \n"\
                            "the hero with the most life remaining.\n");
    else if(ai < 60) printf("Melee: First, monsters adjacent to heroes attack \n"\
                            "adjacent hero, then move as far away from all heroes\n"\
                            "as possible.  Mosters not next to a hero run toward \n"\
                            "closest hero and attack. If multiple heroes are in range \n"\
                            "attack the one with the least health.\n\n"\
                            "Ranged: If within range 5 of a hero, attack weakest hero \n"\
                            "then move as far from all heroes as possible.  If starting \n"\
                            "at range greater than 5, move as close as range 3 and attack \n"\
                            "the hero with the least life remaining.\n");
    else if(ai < 80) printf("Melee: First, monsters adjacent to heroes attack \n"\
                            "adjacent hero, then move as far away from all heroes\n"\
                            "as possible.  Mosters not next to a hero run toward \n"\
                            "closest hero and attack. If multiple heroes are in range \n"\
                            "attack the one with the most health.\n\n"\
                            "Ranged: If within range 5 of a hero, attack weakest hero \n"\
                            "then move as far from all heroes as possible.  If starting \n"\
                            "at range greater than 5, move as close as range 3 and attack \n"\
                            "the hero with the most life remaining.\n");
    else printf("Melee: All monsters rush the closest hero and attack. \n"\
                "Attack the hero with less health if equidistant.\n\n"\
                "Ranged: If within range 5 of a hero, attack weakest hero. \n"\
                "If starting at range greater than 5, move as close as range 3 \n"\
                "and attack the hero with the least life remaining.\n");
    
    
    //make sure you play event cards if you have them on a monster!!
    int i;
    for(i = 0; i<handSize; i++){
        if(hand[i].type == event && overlord.threat >= hand[i].play_cost){
            int extra_threat = overlord.threat - hand[i].play_cost;
            int temp = rand()%EVENT_CHANCE;
            if(extra_threat >= temp){
                    if(strcmp(hand[i].name, "Aim") == 0) printf("\nApply to farthest attacking ranged monster who is within \n"\
                                                                "a maximum of range 6.");
                    else if(strcmp(hand[i].name, "Charge") == 0) printf("\nApply to most powerful monster that cannot come within \n"\
                                                                        "range this turn, but could with double movement.");
                    else if(strcmp(hand[i].name, "Dodge") == 0) printf("\nUse this card during the Heroes' next turn on the hero who \n"\
                                                                       "will roll the most dice.");
                    else if(strcmp(hand[i].name, "Rage") == 0) printf("\nApply to most powerful monster that can attack this turn, \n"\
                                                                      "preferably a ranged monster or one with a special ability \n"\
                                                                      "such as Quick Shot, Sweep, or Breath");
                    else printf("\nPretend this was played at the start of the OLs turn...");
                    
                    playCard(i);
                    
                    break;
            }
        }
    }
    
    if(inplay("DOOM!")) printf("Don't forget, %dx DOOM! cards in play!\n", inplay("DOOM!"));
    keyPause();
}

void take_overlord_turn(){
    //for for loops
    int i;
    
    if(game.countdown == 1){
        int startP = 2;
        int endP = 2;
        int tempP = 2;
        for(endP = 2; endP < strlen(game.cdtext); endP ++){
            if(game.cdtext[endP] == ' '){
                if(endP - startP > PRINT_WIDTH){
                    printf("%.*s\n", tempP-startP, game.cdtext+startP);
                    startP = tempP + 1;
                }
                tempP = endP;
            }
            if(endP == strlen(game.cdtext) - 1){
                if(endP - startP > PRINT_WIDTH){
                    printf("%.*s\n", tempP-startP, game.cdtext+startP);
                    startP = tempP + 1;
                }
                printf("%.*s", endP-startP+1, game.cdtext+startP);
            }
        }
        keyPause();
    }
    if(game.countdown > 0) game.countdown--;
    
    //Step 1: Collect Threat and Draw Cards
    collect_threat(game.players);
    int draw = 2;
    draw += inplay("Evil Genius");
    
    for(i = 0; i<draw; i++){
        draw_card();
        if(deckSize == 0) resuffleDiscard();
    }
        
    //Gather Information
    int open_squares;
    printf("How many nearby squares (current room or adjacent rooms)\n are available to spawn monsters? ");
    scanf("%d%*c", &open_squares);
    printf("\n");
        
    //Dark Charm
    for(i = 0; i<handSize; i++){
        if(hand[i].type == trap && overlord.threat >= hand[i].play_cost){
            int extra_threat = overlord.threat - hand[i].play_cost;
            int temp = rand()%TRAP_CHANCE;
            if(extra_threat >= temp){
                    printf("\nAttack character with the least health within \n"\
                           "range of the weapon that rolls the most dice!!\n"\
                           "(max range 5)\n");
                    playCard(i);
                    break;
            }
        }
    }
        
    //Gain powers
    for(i = 0; i<handSize; i++){
        if(hand[i].type == power && overlord.threat >= hand[i].play_cost){
            int extra_threat = overlord.threat - hand[i].play_cost;
            int temp = rand()%POWER_CHANCE;
            if(extra_threat >= temp){
                    playCard(i);
                    break;
            }
        }
    }
    
    //Step 2: Spawn Monsters
    spawn_monsters(open_squares);
    
    
    //Any Monsters on board?
    int monster_count;
    printf("How many monsters are on the board? ");
    scanf("%d%*c", &monster_count);
    printf("\n");
    
    //Step 3: Activate Monsters
    if(monster_count) activate_monsters();
}

void door_trap(){
    int i;
    for(i = 0; i<handSize; i++){
        if(hand[i].type == trap_door && overlord.threat >= hand[i].play_cost){
            int extra_threat = overlord.threat - hand[i].play_cost;
            int temp = rand()%TRAP_DOOR_CHANCE;
            if(extra_threat >= temp){
                    playCard(i);
                    keyPause();
                    break;
            }
        }
    }
}

void chest_trap(){
    int i;
    for(i = 0; i<handSize; i++){
        if(hand[i].type == trap_chest && overlord.threat >= hand[i].play_cost){
            int extra_threat = overlord.threat - hand[i].play_cost;
            int temp = rand()%TRAP_CHEST_CHANCE;
            if(extra_threat >= temp){
                    playCard(i);
                    keyPause();
                    break;
            }
        }
    }
}

void space_trap(){
    int i;
    for(i = 0; i<handSize; i++){
        if(hand[i].type == trap_space && overlord.threat >= hand[i].play_cost){
            int extra_threat = overlord.threat - hand[i].play_cost;
            int temp = rand()%TRAP_SPACE_CHANCE;
            if(extra_threat >= temp){
                    playCard(i);
                    keyPause();
                    break;
            }
        }
    }
}

void printCardNames(){
    int i;
    for(i = 0; i<fullSize; i++){
        printf("%2d: %-24s", i, fulldeck[i].name);
        if(i%2==1) printf("\n");
    }
}

void setCountdown(char * tag){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    quest_file = fopen(game.questName, "r");
    while ((read = getline(&line, &len, quest_file)) != -1) {
        if(strcmp(line, tag) == 0){
            getline(&line, &len, quest_file);
            while(line[0] != '#'){
                if(strcmp(line, "<Countdown:\n") == 0){
                    getline(&line, &len, quest_file);
                    game.countdown = line[0]-48;
                    strcpy(game.cdtext, line);
                }
                getline(&line, &len, quest_file);
            }
        }
    }
    fclose(quest_file);
    
}

void pickRoom(){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    int avail_rooms = 0;
    int choice = 1;
    int potential_rooms[10];
    int i;
    for(i=0; i<10; i++) potential_rooms[i] = 0;
    
    printf("Which room are you opening?\n");
    quest_file = fopen(game.questName, "r");
    while ((read = getline(&line, &len, quest_file)) != -1) {
        if(strcmp(line, "#MAP:\n") == 0){
            getline(&line, &len, quest_file);
            while(line[0] != '#'){
                while(line[0] == '<'){
                    getline(&line, &len, quest_file);
                    getline(&line, &len, quest_file);
                }
                if(line[0] == 'S'){
                    char buff[2];
                    memcpy(buff, &line[6], 1);
                    if(game.room[atoi(buff)] == 0){
                        potential_rooms[atoi(buff)] = 1;
                        printf("%c\n", *buff);
                        avail_rooms++;
                    }
                }
                else{
                    char buf[2];
                    memcpy(buf, &line[0], 1);
                    if(game.room[atoi(buf)] == 1){
                        char buff[2];
                        memcpy(buff, &line[6], 1);
                        if(game.room[atoi(buff)] == 0){
                            potential_rooms[atoi(buff)] = 1;
                            printf("%c\n", *buff);
                            avail_rooms++;
                        }
                    }
                }
                getline(&line, &len, quest_file);
            }
        }
    }
    fclose(quest_file);
    
    if(avail_rooms > 0){
        int validChoice = 0;
        while(!validChoice){
            printf("Selection: ");
            scanf("%d%*c", &choice);
            printf("\n");
            if(potential_rooms[choice] == 1 && game.room[choice] == 0){
                printf("Opening door %d\n\n", choice);
                validChoice = 1;
                game.room[choice] = 1;
                
                char tag[5];
                tag[0] = '#';"#1:\n";
                tag[1] = choice+48;
                tag[2] = ':';
                tag[3] = '\n';
                tag[4] = 0;
                printSection(tag);
                setCountdown(tag);
            }
            else{
                printf("Invalid Selection\n");
            }
        }
    }
    else{
        printf("\nNo new rooms to open.\n");
    }

}

void open_chest(){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    quest_file = fopen(game.questName, "r");
    while ((read = getline(&line, &len, quest_file)) != -1) {
        if(strcmp(line, "#CHESTS:\n") == 0){
            getline(&line, &len, quest_file);
            while(line[0] != '#'){
                while(line[0] == '<'){
                    if(strcmp(line, "<Copper:\n") == 0){
                        getline(&line, &len, quest_file);
                        printf("Copper Chests:\n");
                        int cnt = 1;
                        while(line[0] != '<' && line[0] != '#'){
                            printf("%d: %s", cnt++, line);
                            getline(&line, &len, quest_file);
                        }
                    }
                    if(strcmp(line, "<Silver:\n") == 0){
                        getline(&line, &len, quest_file);
                        printf("Silver Chests:\n");
                        int cnt = 1;
                        while(line[0] != '<' && line[0] != '#'){
                            printf("%d: %s", cnt++, line);
                            getline(&line, &len, quest_file);
                        }
                    }
                    if(strcmp(line, "<Gold:\n") == 0){
                        getline(&line, &len, quest_file);
                        printf("Gold Chests:\n");
                        int cnt = 1;
                        while(line[0] != '<' && line[0] != '#'){
                            printf("%d: %s", cnt++, line);
                            getline(&line, &len, quest_file);
                        }
                    }
                }
                getline(&line, &len, quest_file);
            }
        }
    }
    fclose(quest_file);
}

void findEncounter(char * tag){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    int no_encounter = 1;
    
    quest_file = fopen(game.questName, "r");
    while ((read = getline(&line, &len, quest_file)) != -1) {
        if(strcmp(line, tag) == 0){
            getline(&line, &len, quest_file);
            while(line[0] != '#' && strcmp(line, "<Encounter:\n") != 0){
                getline(&line, &len, quest_file);
            }
            if(line[0] == '<'){
                getline(&line, &len, quest_file);
                int startP = 0;
                int endP = 0;
                int tempP = 0;
                for(endP = 0; endP < strlen(line); endP ++){
                    if(line[endP] == ' '){
                        if(endP - startP > PRINT_WIDTH){
                            printf("%.*s\n", tempP-startP, line+startP);
                            startP = tempP + 1;
                        }
                        tempP = endP;
                    }
                    if(endP == strlen(line) - 1){
                        if(endP - startP > PRINT_WIDTH){
                            printf("%.*s\n", tempP-startP, line+startP);
                            startP = tempP + 1;
                        }
                        printf("%.*s", endP-startP+1, line+startP);
                    }
                }
                no_encounter = 0;
            }
        }
    }
    fclose(quest_file);
    if(no_encounter) printf("\nNo encounter found in chosen room!!\n");
}

void encounter(){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    char choice;
    
    printf("\nIn which room is the encounter?\n");
    quest_file = fopen(game.questName, "r");
    while ((read = getline(&line, &len, quest_file)) != -1) {
        if(strcmp(line, "#S:\n") == 0){
            printf("S\n");
        }
        else if(line[0] == '#' && line[1]-48 > 0 && line[1]-48 < 10){
            printf("%c\n", line[1]);
        }
    }
    fclose(quest_file);
    printf("Selection: ");
    scanf("%c%*c", &choice);
    
    int room_index = 0;
    if(choice == 'S'){
        room_index = 0;
    }
    else{
        room_index = choice-48;
    }
    
    if(game.room[room_index]){
        char tag[5];
        tag[0] = '#';
        tag[1] = choice;
        tag[2] = ':';
        tag[3] = '\n';
        tag[4] = 0;
        
        findEncounter(tag);
    }
    else{
        printf("\nThat room is not yet open!!\n");
    }
}

void load_game(){

    int i;

    FILE *f = fopen("save.sav", "r");
    if (f == NULL)
    {
        printf("Error saving file!\n");
        exit(1);
    }

    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    getline(&line, &len, f);
    sscanf(line, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%[^|]|%d|%s", &game.players, &game.quest, &game.room[0],  &game.room[1],  &game.room[2],  &game.room[3], \
                                                                 &game.room[4],  &game.room[5],  &game.room[6],  &game.room[7],  &game.room[8],  &game.room[9], \
                                                                 game.questName, &game.countdown, game.cdtext);

    getline(&line, &len, f);
    sscanf(line, "%d|%d", &overlord.threat, &overlord.conquest_tokens);

    for(i=0; i<60; i++){
        char name[100];
        char text[1000];
        getline(&line, &len, f);
        sscanf(line, "%d|%d|%d|%[^|]|%[^|]", &deck[i].play_cost, &deck[i].sell_cost, &deck[i].type, name, text);
        deck[i].name = strdup(name);
        deck[i].text = strdup(text);
    }
    getline(&line, &len, f);
    sscanf(line, "%d\n", &deckSize);

    for(i=0; i<60; i++){
        char name[100];
        char text[1000];
        getline(&line, &len, f);
        sscanf(line, "%d|%d|%d|%[^|]|%[^|]", &fulldeck[i].play_cost, &fulldeck[i].sell_cost, &fulldeck[i].type, name, text);
        fulldeck[i].name = strdup(name);
        fulldeck[i].text = strdup(text);
    }
    getline(&line, &len, f);
    sscanf(line, "%d\n", &fullSize);

    for(i=0; i<15; i++){
        char name[100];
        char text[1000];
        getline(&line, &len, f);
        sscanf(line, "%d|%d|%d|%[^|]|%[^|]", &hand[i].play_cost, &hand[i].sell_cost, &hand[i].type, name, text);
        hand[i].name = strdup(name);
        hand[i].text = strdup(text);
    }
    getline(&line, &len, f);
    sscanf(line, "%d\n", &handSize);

    for(i=0; i<60; i++){
        char name[100];
        char text[1000];
        getline(&line, &len, f);
        sscanf(line, "%d|%d|%d|%[^|]|%[^|]", &discard[i].play_cost, &discard[i].sell_cost, &discard[i].type, name, text);
        discard[i].name = strdup(name);
        discard[i].text = strdup(text);
    }
    getline(&line, &len, f);
    sscanf(line, "%d\n", &discardSize);

    for(i=0; i<20; i++){
        char name[100];
        char text[1000];
        getline(&line, &len, f);
        sscanf(line, "%d|%d|%d|%[^|]|%[^|]", &table[i].play_cost, &table[i].sell_cost, &table[i].type, name, text);
        table[i].name = strdup(name);
        table[i].text = strdup(text);
    }
    getline(&line, &len, f);
    sscanf(line, "%d\n", &tableSize);

    fclose(f);

    printf("\nGame Loaded Successfully!\n");

}

void save_game(){

    int i;

    FILE *f = fopen("save.sav", "w");
    if (f == NULL)
    {
        printf("Error saving file!\n");
        exit(1);
    }
    
    fprintf(f, "%d|%d|", game.players, game.quest);
    for(i=0; i<10; i++) fprintf(f, "%d|", game.room[i]);
    fprintf(f, "%s|%d|%s\n", game.questName, game.countdown, game.cdtext);

    fprintf(f, "%d|%d\n", overlord.threat, overlord.conquest_tokens);

    for(i=0; i<60; i++) fprintf(f, "%d|%d|%d|%s|%s|\n", deck[i].play_cost, deck[i].sell_cost, deck[i].type, deck[i].name, deck[i].text);
    fprintf(f, "%d\n", deckSize);
    for(i=0; i<60; i++) fprintf(f, "%d|%d|%d|%s|%s|\n", fulldeck[i].play_cost, fulldeck[i].sell_cost, fulldeck[i].type, fulldeck[i].name, fulldeck[i].text);
    fprintf(f, "%d\n", fullSize);
    for(i=0; i<15; i++) fprintf(f, "%d|%d|%d|%s|%s|\n", hand[i].play_cost, hand[i].sell_cost, hand[i].type, hand[i].name, hand[i].text);
    fprintf(f, "%d\n", handSize);
    for(i=0; i<60; i++) fprintf(f, "%d|%d|%d|%s|%s|\n", discard[i].play_cost, discard[i].sell_cost, discard[i].type, discard[i].name, discard[i].text);
    fprintf(f, "%d\n", discardSize);
    for(i=0; i<20; i++) fprintf(f, "%d|%d|%d|%s|%s|\n", table[i].play_cost, table[i].sell_cost, table[i].type, table[i].name, table[i].text);
    fprintf(f, "%d\n", tableSize);

    fclose(f);

}

int main(){
	
	printf("\n                                                  .---.  \n");
	printf("                                                 /  .  \\ \n");
	printf("                                                |\\_/|   |\n");
	printf("                                                |   |  /|\n");
	printf("  .---------------------------------------------------' |\n");
	printf(" /  .-.                                                 |\n");
	printf("|  /   \\   Descent Journeys in the Dark (1st Ed.)       |\n");
	printf("| |\\_.  |               Overlord AI                     |\n");
	printf("|\\|  | /|               Version 0.1                     |\n");
	printf("| `---' |                                              / \n");
	printf("|       |---------------------------------------------'  \n");
	printf("\\       |                                                \n");
	printf(" \\     /                                                 \n");
	printf("  `---'                                                  \n");
	printf("\n");


    srand(time(NULL));

    FILE *f = fopen("save.sav", "r");
    if (f == NULL)
    {
        init_game();
    }
    else {
        printf("Saved game found, (C)ontinue, or (N)ew game?");
        
        char game_start;
        scanf("%s%*c", &game_start);
        if(game_start == 'c' || game_start == 'C') load_game();
        else init_game();
    }
    
    int end_game = 1;
    
    while(end_game){
        int choice;
        printf("\n~~~~Select Event:~~~~\n");
        printf("1: Overlord Turn\n");
        printf("2: Hero Movement\n");
        printf("3: Open Door\n");
        printf("4: Open Chest\n");
        printf("5: Encounter (? Token)\n");
        printf("6: OL Discard Pile\n");
        printf("7: OL Stats\n");
        printf("8: Card Info\n");
        printf("9: Save Game\n");
        printf("10: End Game\n");
        printf("Selection: ");
        scanf("%d%*c", &choice);
        printf("\n");
        char quit;
        int card_num;
        
        switch(choice) {
            case(1) :
                take_overlord_turn();
                break;
            case(2) :
                space_trap();
                break;
            case(3) :
                door_trap();
                if(inplay("Hordes of the Things")){
                    printf("Don't forget, %dx Hordes of the Things cards in play!\n", inplay("Hordes of the Things"));
                    int temp = rand()%100;
                    if(temp < 50) printf("Play extra Beastmen\n");
                    else if(temp < 75) printf("Play extra Skeletons\n");
                    else printf("Play extra BaneSpiders\n");
                }
                if(inplay("Brilliant Commander")){ 
                    printf("Don't forget, %dx Brilliant Commander cards in play!\n", inplay("Brilliant Commander"));
                    printf("Upgrade one of whichever there are the most monsters of.\n");
                }
                keyPause();
                pickRoom();
                break;
            case(4) :
                chest_trap();
                open_chest();
                break;
            case(5) :
                encounter();
                break;
            case(6) :
                printf("Discard Pile:\n");
                printCards(discard, discardSize);
                break;
            case(7) :
                //printCards(hand, handSize);
                printf("Powers Played:\n");
                printCards(table, tableSize);
                printf("OL Cards in hand: %d\n", handSize);
                printf("OL Threat: %d\n", overlord.threat);
                //printf("Conquest Tokens: %d\n", overlord.conquest_tokens);
                printf("Cards Remaining: %d \tDiscarded Cards: %d\n", deckSize, discardSize);
                break;
            case(8) :
                printCardNames();
                printf("What card would you like to see?:");
                scanf("%d%*c", &card_num);
                printf("\n");
                printCard(fulldeck[card_num]);
                break;
            case(9) :
				save_game();
				printf("Game Saved!");
				keyPause();
                break;
            case(10) :
                printf("Are you sure you want to quit?\n");
                printf("(y/n)");
                scanf("%s%*c", &quit);
                if(quit == 'y' || quit == 'Y') end_game = 0;
                printf("Do you want to save the current game?\n");
                printf("(y/n)");
                scanf("%s%*c", &quit);
                if(quit == 'y' || quit == 'Y') save_game();
                break;
            default:
                printf("Invalid Choice...");
                break;
        }
        
        //if(overlord.conquest_tokens <= 0) end_game = 0;
    }

    printf("You have been defeated!! (probably)\n");
    
}
