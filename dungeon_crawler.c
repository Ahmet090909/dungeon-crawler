#include <stdio.h> // Inclusie van de standaard input/output bibliotheek voor printf, scanf etc.
#include <stdlib.h> // Inclusie van de standaard bibliotheek voor functies als malloc, free, rand, atoi etc.
#include <string.h> // Inclusie van de bibliotheek om met strings te werken (zoals strcmp, strcpy, strtok).
#include <time.h> // Inclusie van de tijd-bibliotheek om tijdsfuncties te gebruiken, bv. voor random seed.

#define MAX_CONNECTIONS 4 // Definieert het maximaal aantal verbindingen (buren) dat een kamer kan hebben
#define MAX_ROOMS 100 // Definieert het maximaal aantal kamers dat de dungeon kan bevatten

typedef enum { EMPTY, GOBLIN, TROLL, HEAL_POTION, DAMAGE_UP, TREASURE } RoomType; // Enumeratie om de types kamers/inhoud vast te leggen

typedef struct Room { // Struct die een kamer beschrijft
    int id; // Uniek identificatienummer voor de kamer
    RoomType type; // Het type kamer (zoals hierboven gedefinieerd)
    int visited; // Of de speler deze kamer al bezocht heeft (1 = ja, 0 = nee)
    int connection_count; // Hoeveel verbindingen deze kamer heeft met andere kamers
    struct Room* connections[MAX_CONNECTIONS]; // Array van pointers naar andere kamers (buren van deze kamer)
    struct Room* next; // Pointer naar de volgende kamer in een gekoppelde lijst (voor cleanup)
} Room; // Einde van de kamer-struct

typedef struct Player { // Struct die de speler beschrijft
    int hp; // Health points van de speler
    int damage; // Hoeveel schade de speler doet bij een aanval
    Room* current_room; // Pointer naar de kamer waarin de speler zich bevindt
} Player; // Einde van de speler-struct

// --------------------- Monster- & itemfuncties ----------------------
void goblin_attack(Player* p) { // Functie die de gevechtslogica met een goblin uitvoert
    int enemy_hp = 8; // Goblin begint met 8 HP
    int enemy_damage = 3; // Goblin doet 3 schade per aanval
    printf("Er verschijnt een goblin! (HP: %d, DMG: %d)\n", enemy_hp, enemy_damage); // Meldt dat er een goblin verschijnt
    while (enemy_hp > 0 && p->hp > 0) { // Loop zolang zowel goblin als speler leven
        int seq = rand() % 16; // Bepaalt een willekeurige volgorde van aanvallen (4 bits, dus 0-15)
        printf("Aanval volgorde: "); // Print de volgorde van aanvallen
        for (int i = 3; i >= 0; i--) printf("%d", (seq >> i) & 1); // Toont de bits van seq van hoog naar laag
        printf("\n"); // Nieuwe regel na de volgorde
        for (int i = 3; i >= 0 && enemy_hp > 0 && p->hp > 0; i--) { // Doorloopt de 4 aanvallen
            if (((seq >> i) & 1) == 0) { // Bit is 0: goblin valt aan
                p->hp -= enemy_damage; // Speler verliest HP
                printf("Goblin valt aan! Speler HP: %d\n", p->hp); // Print nieuwe HP van speler
            } else { // Bit is 1: speler valt aan
                enemy_hp -= p->damage; // Goblin verliest HP
                printf("Speler valt aan! Goblin HP: %d\n", enemy_hp); // Print nieuwe HP van goblin
            }
        }
    }
    if (enemy_hp <= 0) printf("Goblin is verslagen!\n"); // Goblin dood? Meld dat hij verslagen is
    else printf("Je bent verslagen!\n"); // Anders is de speler dood
}

void troll_attack(Player* p) { // Vergelijkbare functie voor gevecht met een troll
    int enemy_hp = 15; // Troll begint met 15 HP
    int enemy_damage = 5; // Troll doet 5 schade per aanval
    printf("Er verschijnt een troll! (HP: %d, DMG: %d)\n", enemy_hp, enemy_damage); // Meld troll-gevecht
    while (enemy_hp > 0 && p->hp > 0) { // Loop zolang beide leven
        int seq = rand() % 16; // Willekeurige volgorde van aanvallen
        printf("Aanval volgorde: ");
        for (int i = 3; i >= 0; i--) printf("%d", (seq >> i) & 1); // Print volgorde
        printf("\n");
        for (int i = 3; i >= 0 && enemy_hp > 0 && p->hp > 0; i--) {
            if (((seq >> i) & 1) == 0) { // Troll valt aan
                p->hp -= enemy_damage;
                printf("Troll valt aan! Speler HP: %d\n", p->hp);
            } else { // Speler valt aan
                enemy_hp -= p->damage;
                printf("Speler valt aan! Troll HP: %d\n", enemy_hp);
            }
        }
    }
    if (enemy_hp <= 0) printf("Troll is verslagen!\n"); // Troll dood? Meld verslagen
    else printf("Je bent verslagen!\n"); // Speler dood
}

void heal_player(Player* p) { // Functie om speler te genezen
    int before = p->hp; // Onthoud oude HP
    p->hp += 8; // Voeg 8 HP toe
    if (p->hp > 20) p->hp = 20; // Maximaal 20 HP
    printf("Je vindt een heal potion! HP: %d -> %d\n", before, p->hp); // Meld hoeveel HP erbij kwam
}

void damage_up(Player* p) { // Functie om schade van speler te verhogen
    int before = p->damage; // Onthoud oude schade
    p->damage += 2; // Verhoog schade met 2
    printf("Je vindt een zwaard! Damage: %d -> %d\n", before, p->damage); // Meld verandering
}

// --------------------- Dungeon generatie en cleanup ----------------------
Room* create_room(int id, RoomType type) { // Maakt een kamer aan
    Room* room = (Room*)malloc(sizeof(Room)); // Reserveert geheugen voor kamer
    room->id = id; // Zet ID
    room->type = type; // Zet type
    room->visited = 0; // Kamer is nog niet bezocht
    room->connection_count = 0; // Nog geen verbindingen
    for (int i = 0; i < MAX_CONNECTIONS; i++) room->connections[i] = NULL; // Zet alle verbindingen op NULL
    room->next = NULL; // Geen volgende kamer nog
    return room; // Geeft pointer naar nieuwe kamer terug
}

Room* generate_dungeon(int count) { // Genereert een dungeon met gegeven aantal kamers
    if (count > MAX_ROOMS) count = MAX_ROOMS; // Beperk aantal kamers tot maximum
    Room* rooms[MAX_ROOMS] = { 0 }; // Array voor pointers naar kamers
    Room* head = NULL, *prev = NULL; // Begin van de lijst, vorige kamer
    srand((unsigned int)time(NULL)); // Zet random seed op huidige tijd

    // Maak kamers aan
    for (int i = 0; i < count; i++) {
        RoomType t;
        if (i == 0) t = EMPTY; // Eerste kamer is leeg (start)
        else if (i == count - 1) t = TREASURE; // Laatste kamer is schatkamer
        else {
            int r = rand() % 6; // Kies willekeurig kamer-type
            switch (r) {
                case 0: t = GOBLIN; break; // Goblin-kamer
                case 1: t = TROLL; break; // Troll-kamer
                case 2: t = HEAL_POTION; break; // Potion-kamer
                case 3: t = DAMAGE_UP; break; // Zwaard-kamer
                default: t = EMPTY; break; // Anders leeg
            }
        }
        Room* room = create_room(i, t); // Maak kamer aan
        rooms[i] = room; // Sla op in array
        if (!head) head = room; // Eerste kamer wordt head
        if (prev) prev->next = room; // Koppel vorige kamer aan deze
        prev = room; // Update vorige kamer
    }

    // Leg verbindingen aan tussen kamers
    for (int i = 0; i < count; i++) {
        Room* r = rooms[i]; // Pak kamer
        int min_conn = (i == 0 || i == count - 1) ? 1 : 2; // Start/finish minimaal 1 verbinding, anders 2
        int connections = min_conn + rand() % (MAX_CONNECTIONS - min_conn + 1); // Totaal aantal verbindingen
        while (r->connection_count < connections) { // Zolang niet genoeg verbindingen
            int target = rand() % count; // Kies willekeurige kamer als target
            if (target == i || rooms[target]->connection_count >= MAX_CONNECTIONS) continue; // Niet naar zichzelf of te volle kamer
            int already = 0;
            for (int j = 0; j < r->connection_count; j++)
                if (r->connections[j] == rooms[target]) already = 1; // Vermijd dubbele verbindingen
            if (already) continue;
            r->connections[r->connection_count++] = rooms[target]; // Voeg verbinding toe
            rooms[target]->connections[rooms[target]->connection_count++] = r; // Maak verbinding wederzijds
        }
    }
    return head; // Geef begin van dungeon terug
}

void free_dungeon(Room* head) { // Maakt alle kamers weer vrij uit geheugen
    Room* r = head; // Start bij begin
    while (r) { // Loop door alle kamers
        Room* next = r->next; // Onthoud pointer naar volgende kamer
        free(r); // Geef geheugen van huidige kamer vrij
        r = next; // Ga naar volgende kamer
    }
}

// --------------------- Opslaan & laden ----------------------
void save_game(Player* p, Room* head, int room_count, const char* filename) { // Slaat het spel op naar bestand
    FILE* f = fopen(filename, "w"); // Open bestand om te schrijven
    if (!f) { printf("Kan niet opslaan!\n"); return; } // Check of openen lukt
    fprintf(f, "%d %d %d\n", p->hp, p->damage, p->current_room->id); // Sla spelerinfo op
    Room* r = head;
    while (r) { // Loop door alle kamers
        fprintf(f, "%d %d %d %d", r->id, r->type, r->visited, r->connection_count); // Kamerinfo
        for (int i = 0; i < r->connection_count; i++)
            fprintf(f, " %d", r->connections[i]->id); // IDs van verbonden kamers
        fprintf(f, "\n"); // Nieuwe regel voor volgende kamer
        r = r->next;
    }
    fclose(f); // Sluit bestand
    printf("Spel opgeslagen naar %s!\n", filename); // Meld dat opslaan gelukt is
}

Room* find_room_by_id(Room* head, int id) { // Zoek kamer met bepaalde ID in dungeon
    Room* r = head;
    while (r) { // Loop door kamers
        if (r->id == id) return r; // Gevonden
        r = r->next;
    }
    return NULL; // Niet gevonden
}

Room* load_game(Player* p, const char* filename, int* out_room_count) { // Laadt spel uit bestand
    FILE* f = fopen(filename, "r"); // Open bestand om te lezen
    if (!f) { printf("Kan savefile niet vinden!\n"); return NULL; } // Check of openen lukt
    int hp, dmg, cur_id;
    fscanf(f, "%d %d %d\n", &hp, &dmg, &cur_id); // Lees spelerinfo
    p->hp = hp; p->damage = dmg; // Zet spelerwaarden
    Room* head = NULL, *prev = NULL, *rooms[MAX_ROOMS] = { 0 }; // Structs voor kamers
    int id, type, visited, conn_count, room_count = 0;
    char line[256];
    // Eerste pass: kamers aanmaken zonder verbindingen
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "%d %d %d %d", &id, &type, &visited, &conn_count); // Lees kamerinfo
        Room* room = create_room(id, (RoomType)type); // Maak kamer aan
        room->visited = visited; // Zet bezocht-veld
        room->connection_count = conn_count; // Zet aantal verbindingen
        if (!head) head = room;
        if (prev) prev->next = room;
        prev = room;
        rooms[id] = room; // Voeg toe aan array
        ++room_count;
    }
    rewind(f); // Zet bestand terug naar begin
    fgets(line, sizeof(line), f); // Sla spelerregel over
    int idx = 0;
    Room* r = head;
    while (fgets(line, sizeof(line), f) && r) { // Tweede pass: verbindingen aanmaken
        char* pch = strtok(line, " "); // Splitst string op spaties
        int values[4 + MAX_CONNECTIONS] = { 0 };
        int n = 0;
        while (pch != NULL && n < (4 + MAX_CONNECTIONS)) { // Haal getallen uit string
            values[n++] = atoi(pch);
            pch = strtok(NULL, " ");
        }
        // Verbindingen vullen op basis van opgeslagen IDs
        for (int i = 0; i < r->connection_count; i++)
            r->connections[i] = rooms[values[4 + i]];
        r = r->next;
    }
    fclose(f); // Sluit bestand
    *out_room_count = room_count; // Zet aantal kamers
    p->current_room = rooms[cur_id]; // Zet huidige kamer van speler
    printf("Spel geladen uit %s!\n", filename); // Meld dat laden gelukt is
    return head; // Geef dungeon terug
}

// --------------------- Spel Loop ----------------------
void enter_room(Player* player, Room* room, int room_count, Room* head) { // Functie voor wat er gebeurt als je een kamer binnenkomt
    player->current_room = room; // Zet speler in deze kamer
    printf("\n--- Kamer %d ---\n", room->id); // Meld nummer van kamer
    if (!room->visited) { // Als kamer nog niet bezocht
        switch (room->type) { // Afhankelijk van kamertype
            case GOBLIN: goblin_attack(player); break; // Goblin-gevecht
            case TROLL: troll_attack(player); break; // Troll-gevecht
            case HEAL_POTION: heal_player(player); break; // Potion pakken
            case DAMAGE_UP: damage_up(player); break; // Damage omhoog
            case TREASURE:
                printf("Je vond de schat! PROFICIAT!\n"); // Schat gevonden
                exit(0); // Spel winnen
            case EMPTY: printf("Deze kamer is leeg.\n"); break; // Lege kamer
        }
        room->visited = 1; // Zet kamer op bezocht
    } else {
        printf("Deze kamer is al bezocht.\n"); // Kamer al bezocht
    }
    if (player->hp <= 0) { // Speler dood?
        printf("Je bent gestorven! Game over.\n"); // Game over
        exit(1);
    }
    printf("Jouw HP: %d | Damage: %d\n", player->hp, player->damage); // Toon status speler
    printf("Deuren naar kamers: "); // Toon waar je heen kunt
    for (int i = 0; i < room->connection_count; i++)
        printf("%d ", room->connections[i]->id); // Toon ID's van verbonden kamers
    printf("\nKies volgende kamer (of typ 'save' of 'exit'): "); // Vraag om input

    char input[16]; // Buffer voor invoer
    scanf("%15s", input); // Lees invoer van gebruiker
    if (strcmp(input, "save") == 0) { // Wil gebruiker opslaan?
        save_game(player, head, room_count, "savefile.txt"); // Sla op
        enter_room(player, room, room_count, head); // Blijf in deze kamer
        return;
    }
    if (strcmp(input, "exit") == 0) { // Wil speler stoppen?
        printf("Spel gestopt.\n"); // Meld stoppen
        exit(0); // Stop programma
    }
    int keuze = atoi(input); // Zet invoer om naar getal (ID van kamer)
    int idx = -1; // Index van gekozen kamer
    for (int i = 0; i < room->connection_count; i++) { // Zoek of gekozen kamer bestaat
        if (room->connections[i]->id == keuze) {
            idx = i;
            break;
        }
    }
    if (idx != -1) { // Geldige kamer gekozen
        enter_room(player, room->connections[idx], room_count, head); // Ga naar gekozen kamer
    } else {
        printf("Ongeldige keuze!\n"); // Foutmelding bij ongeldige invoer
        enter_room(player, room, room_count, head); // Blijf in huidige kamer
    }
}

// --------------------- Main ----------------------
int main(int argc, char* argv[]) { // Startpunt van het programma
    Player speler; // Maak een speler aan
    speler.hp = 20; // Speler begint met 20 HP
    speler.damage = 5; // Speler begint met 5 damage
    Room* dungeon = NULL; // Pointer naar het dungeonsysteem
    int room_count = 0; // Aantal kamers in dungeon

    if (argc >= 3 && strcmp(argv[1], "load") == 0) { // Wil speler een spel laden?
        dungeon = load_game(&speler, argv[2], &room_count); // Laad spel uit bestand
        if (!dungeon) return 1; // Stoppen als laden faalt
    } else if (argc == 2) { // Nieuw spel starten met opgegeven aantal kamers
        room_count = atoi(argv[1]); // Zet aantal kamers
        if (room_count < 5) { // Te weinig kamers?
            printf("Aantal kamers te laag (minimaal 5)\n"); // Meld fout
            return 1;
        }
        dungeon = generate_dungeon(room_count); // Genereer dungeon
        speler.current_room = dungeon; // Zet speler in startkamer
    } else { // Onjuiste invoer
        printf("Gebruik: %s <aantal kamers>\n", argv[0]); // Toon gebruiksinstructie
        printf("Of: %s load savefile.txt\n", argv[0]);
        return 1; // Stop programma
    }
    enter_room(&speler, speler.current_room, room_count, dungeon); // Start spel in huidige kamer

    free_dungeon(dungeon); // Maak geheugen vrij na afloop van het spel
    return 0; // Eindig programma
}
