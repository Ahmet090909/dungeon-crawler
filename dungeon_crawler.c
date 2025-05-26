#include <stdio.h> // Nodig voor input/output functies zoals printf en scanf
#include <stdlib.h> // Nodig voor memory allocatie (malloc, free), atoi, rand, srand
#include <string.h> // Nodig voor string functies zoals strcmp, strtok
#include <time.h>   // Nodig om srand te initialiseren voor random getallen

#define MAX_CONNECTIONS 4 // Maximaal aantal deuren per kamer, bepaalt de breedte van de dungeon
#define MAX_ROOMS 100     // Maximaal aantal kamers dat de dungeon mag bevatten

// Enum type om de inhoud van een kamer te specificeren
typedef enum { EMPTY, GOBLIN, TROLL, HEAL_POTION, DAMAGE_UP, TREASURE } RoomType;

// Struct definitie voor een kamer in de dungeon
typedef struct Room {
    int id; // Uniek nummer per kamer, zodat je ze makkelijk kan adresseren en opslaan
    RoomType type; // Wat zit er in de kamer? (zie enum hierboven)
    int visited; // 0 of 1: is de kamer al bezocht? Nodig voor eenmalige triggers en visuele feedback
    int connection_count; // Hoeveel deuren zijn er uit deze kamer?
    struct Room* connections[MAX_CONNECTIONS]; // Array van pointers naar maximaal 4 andere kamers
    struct Room* next; // Pointer naar de volgende kamer in een linked list, handig voor cleanup en opslag
} Room;

// Struct definitie voor de speler
typedef struct Player {
    int hp; // Health points van de speler; als dit 0 wordt, is de speler dood
    int damage; // Hoeveel schade doet de speler per aanval
    Room* current_room; // In welke kamer bevindt de speler zich nu? (pointer)
} Player;

// --------------------- Monster- & itemfuncties ----------------------

// Functie voor het gevecht met een goblin.
// Gebruikt bitwise random volgorde om te bepalen wie wanneer aanvalt.
void goblin_attack(Player* p) {
    int enemy_hp = 8; // Goblin begint met 8 HP
    int enemy_damage = 3; // Goblin doet 3 schade per aanval
    printf("Er verschijnt een goblin! (HP: %d, DMG: %d)\n", enemy_hp, enemy_damage);
    while (enemy_hp > 0 && p->hp > 0) { // Zolang beiden nog leven
        int seq = rand() % 16; // Genereer random getal tussen 0-15 (4 bits) 
        printf("Aanval volgorde: ");
        for (int i = 3; i >= 0; i--) printf("%d", (seq >> i) & 1); // Print aanvalvolgorde in bits
        printf("\n");
        // Voor elke bit: 0 = goblin valt aan, 1 = speler valt aan
        for (int i = 3; i >= 0 && enemy_hp > 0 && p->hp > 0; i--) {
            if (((seq >> i) & 1) == 0) { // Bit is 0: goblin valt aan
                p->hp -= enemy_damage; // Verminder HP van speler
                printf("Goblin valt aan! Speler HP: %d\n", p->hp);
            } else { // Bit is 1: speler valt aan
                enemy_hp -= p->damage; // Verminder HP van goblin
                printf("Speler valt aan! Goblin HP: %d\n", enemy_hp);
            }
        }
    }
    if (enemy_hp <= 0) printf("Goblin is verslagen!\n"); // Goblin dood? Print overwinning
    else printf("Je bent verslagen!\n"); // Speler dood? Print game over
}

// Functie voor het gevecht met een troll (sterker monster)
void troll_attack(Player* p) {
    int enemy_hp = 15; // Troll heeft meer HP
    int enemy_damage = 5; // Troll doet meer schade
    printf("Er verschijnt een troll! (HP: %d, DMG: %d)\n", enemy_hp, enemy_damage);
    while (enemy_hp > 0 && p->hp > 0) { // Zelfde principe als goblin
        int seq = rand() % 16; // 4 bits random volgorde
        printf("Aanval volgorde: ");
        for (int i = 3; i >= 0; i--) printf("%d", (seq >> i) & 1);
        printf("\n");
        for (int i = 3; i >= 0 && enemy_hp > 0 && p->hp > 0; i--) {
            if (((seq >> i) & 1) == 0) {
                p->hp -= enemy_damage;
                printf("Troll valt aan! Speler HP: %d\n", p->hp);
            } else {
                enemy_hp -= p->damage;
                printf("Speler valt aan! Troll HP: %d\n", enemy_hp);
            }
        }
    }
    if (enemy_hp <= 0) printf("Troll is verslagen!\n");
    else printf("Je bent verslagen!\n");
}

// Functie voor het healen van de speler door het pakken van een heal potion
void heal_player(Player* p) {
    int before = p->hp; // Sla vorige HP op voor feedback
    p->hp += 8; // Voeg 8 HP toe
    if (p->hp > 20) p->hp = 20; // HP mag niet boven 20 uitkomen
    printf("Je vindt een heal potion! HP: %d -> %d\n", before, p->hp); // Toon effect
}

// Functie voor het versterken van de speler met een damage-up item
void damage_up(Player* p) {
    int before = p->damage; // Sla vorige damage op
    p->damage += 2; // Damage +2
    printf("Je vindt een zwaard! Damage: %d -> %d\n", before, p->damage); // Toon effect
}

// --------------------- Dungeon generatie en cleanup ----------------------

// Functie voor het aanmaken van een kamer op de heap
Room* create_room(int id, RoomType type) {
    Room* room = (Room*)malloc(sizeof(Room)); // Reserveer geheugen voor kamer heap niet stack 
    room->id = id; // Zet kamer id
    room->type = type; // Zet kamer type (monster, item, leeg, schat)
    room->visited = 0; // Nog niet bezocht
    room->connection_count = 0; // Nog geen verbindingen
    for (int i = 0; i < MAX_CONNECTIONS; i++) room->connections[i] = NULL; // Zet alle verbindingen op NULL
    room->next = NULL; // Volgende kamer is nog niet bekend
    return room; // Geef pointer naar nieuwe kamer terug
}

// Functie voor het genereren van de volledige dungeon (graph structuur)
Room* generate_dungeon(int count) {
    if (count > MAX_ROOMS) count = MAX_ROOMS; // Niet meer dan MAX_ROOMS kamers
    Room* rooms[MAX_ROOMS] = { 0 }; // Array van pointers naar kamers
    Room* head = NULL, *prev = NULL; // Head van linked list, en vorige kamer pointer
    srand((unsigned int)time(NULL)); // Initialiseer random generator met huidige tijd

    // Maak alle kamers aan en koppel ze in een linked list
    for (int i = 0; i < count; i++) {
        RoomType t;
        if (i == 0) t = EMPTY; // Eerste kamer altijd leeg
        else if (i == count - 1) t = TREASURE; // Laatste kamer altijd schat
        else {
            int r = rand() % 6; // Kies type voor kamer
            switch (r) {
                case 0: t = GOBLIN; break;
                case 1: t = TROLL; break;
                case 2: t = HEAL_POTION; break;
                case 3: t = DAMAGE_UP; break;
                default: t = EMPTY; break; // Rest leeg
            }
        }
        Room* room = create_room(i, t); // Maak kamer aan op heap
        rooms[i] = room; // Bewaar pointer in array zodat we makkelijk verbindingen kunnen leggen
        if (!head) head = room; // Eerste kamer als head
        if (prev) prev->next = room; // Koppel vorige kamer aan deze
        prev = room; // Update vorige kamer
    }

    // Leg verbindingen tussen kamers voor de adjacency list
    for (int i = 0; i < count; i++) {
        Room* r = rooms[i]; // Pak kamer i
        int min_conn = (i == 0 || i == count - 1) ? 1 : 2; // Start/eindkamer altijd minimaal 1 deur, anderen minimaal 2
        int connections = min_conn + rand() % (MAX_CONNECTIONS - min_conn + 1); // Bepaal random aantal verbindingen (tussen min en max)
        while (r->connection_count < connections) { // Zolang er nog verbindingen bij moeten
            int target = rand() % count; // Kies random andere kamer
            if (target == i || rooms[target]->connection_count >= MAX_CONNECTIONS) continue; // Geen verbinding naar zichzelf, of als doelkamer vol zit
            int already = 0;
            for (int j = 0; j < r->connection_count; j++)
                if (r->connections[j] == rooms[target]) already = 1; // Check of verbinding al bestaat
            if (already) continue; // Skip dubbele verbindingen
            r->connections[r->connection_count++] = rooms[target]; // Voeg verbinding toe naar doelkamer
            rooms[target]->connections[rooms[target]->connection_count++] = r; // Maak verbinding terug (bidirectioneel)
        }
    }
    return head; // Return pointer naar eerste kamer (head van linked list)
}

// Functie om alle kamers netjes van het geheugen te verwijderen (heap opruimen)
void free_dungeon(Room* head) {
    Room* r = head; // Start bij eerste kamer
    while (r) { // Loop door alle kamers
        Room* next = r->next; // Onthoud volgende kamer
        free(r); // Verwijder huidige kamer van heap
        r = next; // Ga naar volgende kamer
    }
}

// --------------------- Opslaan & laden ----------------------

// Functie om het spel op te slaan naar een bestand
void save_game(Player* p, Room* head, int room_count, const char* filename) {
    FILE* f = fopen(filename, "w"); // Open bestand in schrijfmodus
    if (!f) { printf("Kan niet opslaan!\n"); return; }
    // Sla eerst speler op: hp, damage, id van huidige kamer
    fprintf(f, "%d %d %d\n", p->hp, p->damage, p->current_room->id);
    Room* r = head; // Loop door alle kamers
    while (r) {
        // Voor elke kamer: id, type, visited, aantal verbindingen, en ids van verbonden kamers
        fprintf(f, "%d %d %d %d", r->id, r->type, r->visited, r->connection_count);
        for (int i = 0; i < r->connection_count; i++)
            fprintf(f, " %d", r->connections[i]->id);
        fprintf(f, "\n"); // Nieuwe regel na elke kamer
        r = r->next;
    }
    fclose(f); // Sluit bestand
    printf("Spel opgeslagen naar %s!\n", filename);
}

// Vind een kamer met een bepaald id in de linked list
Room* find_room_by_id(Room* head, int id) {
    Room* r = head; // Begin bij eerste kamer
    while (r) { // Loop tot einde
        if (r->id == id) return r; // Gevonden? Return pointer
        r = r->next;
    }
    return NULL; // Niet gevonden? Return NULL
}

// Functie om een opgeslagen spel weer in te laden
Room* load_game(Player* p, const char* filename, int* out_room_count) {
    FILE* f = fopen(filename, "r"); // Open bestand in leesmodus
    if (!f) { printf("Kan savefile niet vinden!\n"); return NULL; }
    int hp, dmg, cur_id;
    fscanf(f, "%d %d %d\n", &hp, &dmg, &cur_id); // Lees speler info
    p->hp = hp; p->damage = dmg; // Zet speler stats
    Room* head = NULL, *prev = NULL, *rooms[MAX_ROOMS] = { 0 };
    int id, type, visited, conn_count, room_count = 0;
    char line[256];
    // Eerst alle kamers aanmaken
    while (fgets(line, sizeof(line), f)) { // Lees elke regel (kamer)
        sscanf(line, "%d %d %d %d", &id, &type, &visited, &conn_count); // Parse kamerinfo
        Room* room = create_room(id, (RoomType)type); // Maak kamer aan
        room->visited = visited; // Zet bezocht-flag
        room->connection_count = conn_count; // Zet aantal verbindingen
        if (!head) head = room; // Eerste kamer als head
        if (prev) prev->next = room; // Koppel aan vorige kamer
        prev = room; // Update vorige
        rooms[id] = room; // Bewaar pointer op basis van id voor later
        ++room_count;
    }
    rewind(f); // Zet bestand terug naar begin
    fgets(line, sizeof(line), f); // Sla speler-regel over
    int idx = 0;
    Room* r = head;
    // Tweede pass: verbindingen herstellen via ids
    while (fgets(line, sizeof(line), f) && r) {
        char* pch = strtok(line, " ");
        int values[4 + MAX_CONNECTIONS] = { 0 }; // Maximaal 4 verbindingen
        int n = 0;
        while (pch != NULL && n < (4 + MAX_CONNECTIONS)) {
            values[n++] = atoi(pch); // Zet waarden in array
            pch = strtok(NULL, " ");
        }
        // Vul connecties in per kamer
        for (int i = 0; i < r->connection_count; i++)
            r->connections[i] = rooms[values[4 + i]];
        r = r->next;
    }
    fclose(f); // Sluit bestand
    *out_room_count = room_count; // Zet output-parameter voor kamertelling
    p->current_room = rooms[cur_id]; // Zet speler in de juiste kamer
    printf("Spel geladen uit %s!\n", filename);
    return head; // Return pointer naar head van dungeon
}

// --------------------- Spel Loop ----------------------

// Functie die wordt aangeroepen telkens de speler een kamer betreedt
void enter_room(Player* player, Room* room, int room_count, Room* head) {
    player->current_room = room; // Zet speler in deze kamer
    printf("\n--- Kamer %d ---\n", room->id); // Toon kamernummer
    // Eerste keer in deze kamer? Voer inhoud uit
    if (!room->visited) {
        switch (room->type) {
            case GOBLIN: goblin_attack(player); break; // Start gevecht met goblin
            case TROLL: troll_attack(player); break;   // Start gevecht met troll
            case HEAL_POTION: heal_player(player); break; // Heal de speler
            case DAMAGE_UP: damage_up(player); break;     // Verhoog damage van speler
            case TREASURE:
                printf("Je vond de schat! PROFICIAT!\n"); // Spel gewonnen!
                exit(0);
            case EMPTY: printf("Deze kamer is leeg.\n"); break; // Geen effect
        }
        room->visited = 1; // Markeer als bezocht
    } else {
        printf("Deze kamer is al bezocht.\n"); // Toon dat kamer reeds bezocht is
    }
    if (player->hp <= 0) { // Is de speler dood?
        printf("Je bent gestorven! Game over.\n");
        exit(1);
    }
    // Toon status van speler en mogelijke uitgangen
    printf("Jouw HP: %d | Damage: %d\n", player->hp, player->damage);
    printf("Deuren naar kamers: ");
    for (int i = 0; i < room->connection_count; i++)
        printf("%d ", room->connections[i]->id); // Toon id's van verbonden kamers
    printf("\nKies volgende kamer (of typ 'save' of 'exit'): ");

    char input[16];
    scanf("%15s", input); // Lees input van gebruiker
    if (strcmp(input, "save") == 0) { // Opslaan?
        save_game(player, head, room_count, "savefile.txt");
        enter_room(player, room, room_count, head); // Blijf in huidige kamer na opslaan
        return;
    }
    if (strcmp(input, "exit") == 0) { // Afsluiten?
        printf("Spel gestopt.\n");
        exit(0);
    }
    int keuze = atoi(input); // Zet gekozen kamernummer om in een int
    int idx = -1;
    // Zoek index van gekozen kamer in de lijst van verbindingen
    for (int i = 0; i < room->connection_count; i++) {
        if (room->connections[i]->id == keuze) {
            idx = i;
            break;
        }
    }
    if (idx != -1) { // Geldige keuze?
        enter_room(player, room->connections[idx], room_count, head); // Ga naar gekozen kamer
    } else {
        printf("Ongeldige keuze!\n"); // Onbekend kamernummer gekozen
        enter_room(player, room, room_count, head); // Blijf in huidige kamer
    }
}

// --------------------- Main ----------------------

// Entry-point van het programma, handelt opstarten, nieuw spel of laden af
int main(int argc, char* argv[]) {
    Player speler; // Maak speler aan
    speler.hp = 20; // Start HP
    speler.damage = 5; // Start damage
    Room* dungeon = NULL; // Pointer naar eerste kamer
    int room_count = 0; // Aantal kamers in deze dungeon

    // Check command line argumenten om te bepalen of we nieuw spel starten of laden
    if (argc >= 3 && strcmp(argv[1], "load") == 0) {
        dungeon = load_game(&speler, argv[2], &room_count); // Laad spel uit bestand
        if (!dungeon) return 1; // Laden mislukt
    } else if (argc == 2) {
        room_count = atoi(argv[1]); // Aantal kamers uit argument
        if (room_count < 5) {
            printf("Aantal kamers te laag (minimaal 5)\n");
            return 1;
        }
        dungeon = generate_dungeon(room_count); // Genereer nieuwe dungeon
        speler.current_room = dungeon; // Zet speler in eerste kamer
    } else {
        printf("Gebruik: %s <aantal kamers>\n", argv[0]); // Toon gebruik als geen argumenten
        printf("Of: %s load savefile.txt\n", argv[0]);
        return 1;
    }
    enter_room(&speler, speler.current_room, room_count, dungeon); // Start spel

    free_dungeon(dungeon); // Opruimen van geheugen na afloop (optioneel, want exit() wordt meestal eerder aangeroepen)
    return 0; // Einde programma
} 
