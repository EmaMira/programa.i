#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <mmsystem.h> // <--- NOVA BIBLIOTECA PARA ÁUDIO

// Diz para o compilador do VS Code ligar a biblioteca de som do Windows
#pragma comment(lib, "winmm.lib")

#define LARGURA 80
#define ALTURA 22
#define MAX_INIMIGOS 5
#define MAX_DIVER 10
#define ATRASO_TIQUE 60

// Caractere do jogo 
#define CHAR_MAR  ' '
#define CHAR_AGUA  '~'
#define CHAR_CHAO  '='

// Estrutura do Jogo
typedef struct {
    int x, y;
    int oxigenio;
    int vidas;
    int pontos;
    int mergulhadores_salvos;
} Submarino;

typedef struct {
int x,y;
int ativo;
} Entidade;

typedef struct {
int x,y;
int ativo;
} Tiro;

//Globais seguindo o padrão dos seus códigos
Submarino player;
Tiro tiro;
Entidade inimigos[MAX_INIMIGOS];
Entidade divers[MAX_DIVER];

int frame = 0;
int game_over = 0;

HANDLE hConsole;
CHAR_INFO  consoleBuffer[LARGURA*ALTURA];
COORD bufferSize = {LARGURA, ALTURA};
COORD characterPos = {0,0};
SMALL_RECT consoleWriteArea = { 0, 0, LARGURA - 1, ALTURA - 1};

// Inicializa ou reinicializa o estado do jogo 
void reset () {
player.x = 10;
player.y = 10;
player.oxigenio = 100;
player.vidas = 3;
player.pontos = 0;
player.mergulhadores_salvos = 0;

tiro.ativo = 0;
for (int i= 0; i< MAX_INIMIGOS; i++) inimigos [i].ativo =0;
for (int i= 0; i< MAX_DIVER; i++) divers[i].ativo = 0;

game_over= 0;
frame=0;
}

// Desenha no buffer e envia para a tela de uma vez só (evita cintilação)
void desenha_tela() {
//1. Limpar o buffer e desenhar o cenario de fundo 
for (int y=0; y < ALTURA; y++) {
for (int x=0; x < LARGURA; x++) {
int idx = y* LARGURA + x;
if (y==2){
consoleBuffer[idx] .Char.AsciiChar = CHAR_AGUA;
consoleBuffer[idx] .Attributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY;     
} else if (y == ALTURA -1) {
consoleBuffer[idx] .Char.AsciiChar = CHAR_CHAO;
consoleBuffer[idx] .Attributes = FOREGROUND_RED | FOREGROUND_GREEN;
//amarelo/ marrom   
}else {
consoleBuffer[idx] .Char.AsciiChar = CHAR_MAR;
consoleBuffer[idx] .Attributes = FOREGROUND_BLUE;

}    
}    
}

//2. Desenhar o jogador (Submarino: =[OOO]>)
if (player.y >= 0 && player.y < ALTURA) {
    int idx = player.y * LARGURA + player.x;
    
    // CORREÇÃO: O sinal deve ser < LARGURA (menor que a largura) para não dar erro na tela
    if (player.x >= 0 && player.x + 6 < LARGURA) {
        
        // Desenha o formato do submarino caractere por caractere
        consoleBuffer[idx].Char.AsciiChar     = '=';
        consoleBuffer[idx + 1].Char.AsciiChar = '[';
        consoleBuffer[idx + 2].Char.AsciiChar = 'O';
        consoleBuffer[idx + 3].Char.AsciiChar = 'O';
        consoleBuffer[idx + 4].Char.AsciiChar = 'O';
        consoleBuffer[idx + 5].Char.AsciiChar = ']';
        consoleBuffer[idx + 6].Char.AsciiChar = '>';

        // Aplica a cor verde em cada um dos 7 caracteres do submarino
        WORD cor_sub = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        consoleBuffer[idx].Attributes     = cor_sub;
        consoleBuffer[idx + 1].Attributes = cor_sub;
        consoleBuffer[idx + 2].Attributes = cor_sub;
        consoleBuffer[idx + 3].Attributes = cor_sub;
        consoleBuffer[idx + 4].Attributes = cor_sub;
        consoleBuffer[idx + 5].Attributes = cor_sub;
        consoleBuffer[idx + 6].Attributes = cor_sub;
    }
}

// 3.Desenha o Tiro
if(tiro.ativo) {
int idx = tiro.y * LARGURA + tiro.x;
consoleBuffer[idx]. Char.AsciiChar = '-';
consoleBuffer[idx]. Char.AsciiChar = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
}

//4.Desenhar Inimigos (<x>)
for (int i = 0; i < MAX_INIMIGOS; i++ ) {
if (inimigos [i].ativo) {
int idx = inimigos [i].y * LARGURA + inimigos [i].x;
if (inimigos[i] .x >= 0 && inimigos [i] .x + 2 > LARGURA) {
consoleBuffer [idx] .Char .AsciiChar = '<';
consoleBuffer [idx + 1] .Char.AsciiChar = 'X'; 
consoleBuffer [idx + 2] .Char.AsciiChar = '>';
WORD cor_ini = FOREGROUND_RED | FOREGROUND_INTENSITY;

consoleBuffer [idx] . Attributes = cor_ini;
consoleBuffer [idx + 1] .Attributes = cor_ini;
consoleBuffer [idx + 2] .Attributes = cor_ini;
}
}
}

//5.Desenhar mergulhadores (o/)
for (int i = 0; i < MAX_DIVER; i++) {
if (divers [i].ativo) {
int idx = divers [i].y * LARGURA + divers [i].x;
if (divers[i].x  >= 0 && divers [i].x + 1 > LARGURA){
consoleBuffer [idx] .Char .AsciiChar = 'O';
consoleBuffer  [idx] . Char .AsciiChar = '/';

WORD cor_div = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
consoleBuffer [idx]. Attributes = cor_div;
consoleBuffer [idx + 1] .Attributes = cor_div;
}  
}
}

// Envia o buffer renderizado para o wwindows Console 
WriteConsoleOutputA (hConsole, consoleBuffer, bufferSize, characterPos, &consoleWriteArea);

//Posiciona o cursor abaixo da matriz do jogo para HUD de texto 
SetConsoleCursorPosition (hConsole, (COORD){0, ALTURA});
printf ("PONTOS: %05d | VIDAS:  %d | MERGULHADORES: %d/6 | OXIGENIO:  %3d%% \n",
player.pontos, player.vidas, player.mergulhadores_salvos, player.oxigenio);

if (game_over) {
printf ("ACABOU O JOGO! PERDEDOR!!! Aperta R comecar denovo. \n");
} else {
printf ("Controles: W, A, S, D para mexer | ESPAÇO  para dar os tiros \n ");
}
}

//ATUALIZA AS POSIÇÕES, COLISÕES E LÓGICAS DE JOGO
void atualiza_tela () {
    if (game_over) {
        PlaySound(NULL, 0, 0); // Para a música de fundo
        PlaySound(TEXT("defeat.wav"), NULL, SND_FILENAME | SND_ASYNC);
        return; // Trava as atualizações se o jogo acabou
    }

    frame++;
//logica do oxigênio
if (player.y > 2) {
// Consome oxigênio debaixo da d'agua
if (frame % 12 == 0 && player.oxigenio > 0) player.oxigenio--;
if (player.oxigenio <= 0) {
player.vidas --;
player.oxigenio = 100;
player.y = 2;


// força para retornar
if (player.vidas <= 0 ) game_over =1;

}
}else {

// NA SUPERFICIE REABASTECE E DESCARREGA OS MERGULHADORES SALVOS
player.oxigenio = 100;
if (player.mergulhadores_salvos > 0) {
player.pontos += player.mergulhadores_salvos *100;
player.mergulhadores_salvos = 0;

}
}

// COMO MEXE O TIRO 

if(tiro.ativo) {
tiro.x += 2;
if (tiro.x >= LARGURA - 1) tiro.ativo = 0;
}

// repetir inimigos no frame 
if (frame %20 == 0) {
for (int i = 0; i < MAX_INIMIGOS; i++) {
if (!inimigos[i].ativo) {

inimigos [i] .ativo = 1;
inimigos [i] .x = LARGURA -4;
inimigos [i] .y = 4 + (rand () % (ALTURA - 6));
break;
}
}
}

// QUANDO MEXE E DE FRENTE BATE COM OS INIMIGOS
for (int i = 0; i< MAX_INIMIGOS; i++) {
if ( frame % 2 == 0 )  inimigos [i].x--;

//Velocidade de movimento do inimigo
if (inimigos [i].x <= 0) inimigos [i] . ativo = 0;

// colisão do tiro -> tiro INIMIGO
if (tiro.ativo && tiro.y == inimigos [i].y && (tiro.x >= inimigos [i]. x && tiro.x <= inimigos [i].x +2 )) {
inimigos [i] .ativo = 0;
tiro.ativo = 0;
player.pontos += 20;
}

// Batida  do jogador -> inimigo
if (player.y == inimigos [i].y && (player.x + 3 >= inimigos [i].x && player.x <= inimigos [i].x + 2)) {
player.vidas --;
inimigos [i].ativo = 0;
player.oxigenio = 100;
if (player.vidas <= 0) game_over = 1;
//repetir mergulhadores no frame
// Criação de novos mergulhadores periodicamente
if (frame % 50 == 0) {
    for (int i = 0; i < MAX_DIVER; i++) {
        if (!divers[i].ativo) {
            divers[i].ativo = 1;
            divers[i].x = LARGURA - 3;
            divers[i].y = 4 + (rand() % (ALTURA - 6));
            break;
        }
    }
}    

// movimento e resgate dos mergulhadores
for (int i = 0; i < MAX_DIVER; i++) {
    if (divers[i].ativo) {
        // Mover mergulhador para a esquerda
        if (frame % 3 == 0) {
            divers[i].x--;
        }
        
        // Se sumir da tela pela esquerda, desativa
        if (divers[i].x <= 0) {
            divers[i].ativo = 0;
        }
        
        // Lógica de colisão corrigida com o jogador para resgate e pontuação
        if (player.y == divers[i].y && (player.x + 3 >= divers[i].x && player.x <= divers[i].x + 1)) {
            divers[i].ativo = 0;
            if (player.mergulhadores_salvos < 6) {
                player.mergulhadores_salvos++;
                player.pontos += 50;
            }
        }
    }
} // FIM DO FOR DOS MERGULHADORES

// Comentário corrigido com barras normais (//)
// comandos usando API assíncrona do Windows
void comandos () {
    if (game_over) {
        if (GetAsyncKeyState('R') & 0x8000) {
            reset();
        }
        return;
    }

    if ((GetAsyncKeyState('W') & 0x8000) && player.y > 2) {
        player.y--;
    }   

    if ((GetAsyncKeyState('S') & 0x8000) && player.y < ALTURA - 2) {
        player.y++;
    }                          

    if ((GetAsyncKeyState('A') & 0x8000) && player.x > 1) {
        player.x--;
    }

    if ((GetAsyncKeyState('D') & 0x8000) && player.x < LARGURA - 5) {
        player.x++;
    }

    if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && !tiro.ativo) {
        tiro.ativo = 1;
        tiro.x = player.x + 4;
        tiro.y = player.y;
    }
}

int main () {
    srand(time(NULL));
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // ocultar o cursor do console piscante
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    reset(); 
    PlaySound(TEXT("fundo.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
    
    //loop principal do jogo
    while (1) {
        comandos();
        atualiza_tela();
        desenha_tela(); 
        Sleep(ATRASO_TIQUE);
    }
    return 0;
}