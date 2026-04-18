#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#define TAM_LINHA 100
#define TAM_ENDERECO 10

// ----------------------------- Structs -----------------------------------

typedef struct {
    int on;             // Line valid bit
    int tag;            // Tag bits
    int dirtybit;       // Dirty bit for write-back policy
    unsigned long lru;  // LRU counter for replacement
} Linha;

typedef struct {
    Linha* linhas;      // Array of lines in this set
} CacheConj;

typedef struct {
    int politica_write;     // Write policy (0: write-through, 1: write-back)
    int politica_subs;      // Replacement policy (0: random, 1: LRU)
    CacheConj* conjuntos;   // Array of sets
    int nro_conjutos;       // Number of sets in the cache
    int tamanho_total_cache;// Total cache size in bytes
    int vias_associacao;    // Associativity (lines per set)
    int tam_linha;          // Block size in bytes
} Cache;

typedef struct {
    int tempo_read_mem;
    int tempo_write_mem;
    int tempo_hit;

    int cont_read_total;
    int cont_write_total;
    int cont_mem_read;
    int cont_mem_write;

    int hits_write;
    int hits_read;

    unsigned long cont_acessos;  // Access counter for LRU
} Header;

Header header;

// ---------------------------- Function Prototypes ------------------------

void setHeader();
int busca_linha(CacheConj* conj, int associacao, int tag);
int busca_substituto(Cache* cache, CacheConj* conjunto);
void varredura_final(Cache* cache);
void simula_cache(Cache* cache, unsigned int endereco, int op);
void print_config(Cache* c);
void print_resultado(Cache* c);

// ---------------------------- Function Definitions ------------------------

void setHeader() {
    header.cont_read_total = 0;
    header.cont_write_total = 0;
    header.cont_mem_read = 0;
    header.cont_mem_write = 0;
    header.hits_write = 0;
    header.hits_read = 0;
    header.cont_acessos = 0;
}

int busca_linha(CacheConj* conj, int associacao, int tag) {
    for (int i = 0; i < associacao; i++) {
        if (conj->linhas[i].on && conj->linhas[i].tag == tag) {
            return i;
        }
    }
    return -1;
}

int busca_substituto(Cache* cache, CacheConj* conjunto) {
    if (cache->politica_subs == 1) { // LRU
        unsigned long menor = ULONG_MAX;
        int substituto = 0;

        for (int i = 0; i < cache->vias_associacao; i++) {
            if (!conjunto->linhas[i].on) {
                return i; // Empty line found
            }
            if (conjunto->linhas[i].lru < menor) {
                menor = conjunto->linhas[i].lru;
                substituto = i;
            }
        }
        return substituto;
    } else { // Random
        return rand() % cache->vias_associacao;
    }
}

void varredura_final(Cache* cache) {
    for (int i = 0; i < cache->nro_conjutos; i++) {
        for (int j = 0; j < cache->vias_associacao; j++) {
            Linha* linha = &cache->conjuntos[i].linhas[j];
            if (linha->on && linha->dirtybit) {
                header.cont_mem_write++;
                linha->dirtybit = 0;
            }
        }
    }
}

void simula_cache(Cache* cache, unsigned int endereco, int op) {
    header.cont_acessos++;

    // Calculate bit sizes
    int palavra_bits = (int)log2(cache->tam_linha);
    int conj_index_bits = (int)log2(cache->nro_conjutos);

    // Extract index and tag from address
    int index = (endereco >> palavra_bits) & (cache->nro_conjutos - 1);
    int tag = endereco >> (palavra_bits + conj_index_bits);

    CacheConj* conj = &cache->conjuntos[index];
    int index_linha = busca_linha(conj, cache->vias_associacao, tag);

    if (op == 'W') {
        header.cont_write_total++;

        if (index_linha != -1) { // HIT
            header.hits_write++;

            if (cache->politica_write == 0) { // write-through
                header.cont_mem_write++;
            } else { // write-back
                conj->linhas[index_linha].dirtybit = 1;
            }

            conj->linhas[index_linha].lru = header.cont_acessos;
        } else { // MISS
            header.cont_mem_read++;

            int subs = busca_substituto(cache, conj);

            Linha* linha_subs = &conj->linhas[subs];
            if (linha_subs->on && linha_subs->dirtybit && cache->politica_write == 1) {
                header.cont_mem_write++;
            }

            linha_subs->tag = tag;
            linha_subs->on = 1;
            linha_subs->lru = header.cont_acessos;

            if (cache->politica_write == 0) {
                header.cont_mem_write++;
                linha_subs->dirtybit = 0;
            } else {
                linha_subs->dirtybit = 1;
            }
        }
    } else if (op == 'R') {
        header.cont_read_total++;

        if (index_linha != -1) { // HIT
            header.hits_read++;
            conj->linhas[index_linha].lru = header.cont_acessos;
        } else { // MISS
            header.cont_mem_read++;

            int subs = busca_substituto(cache, conj);
            Linha* linha_subs = &conj->linhas[subs];

            if (linha_subs->on && linha_subs->dirtybit && cache->politica_write == 1) {
                header.cont_mem_write++;
            }

            linha_subs->tag = tag;
            linha_subs->on = 1;
            linha_subs->dirtybit = 0;
            linha_subs->lru = header.cont_acessos;
        }
    }
}

void print_config(Cache* c) {
    printf("--CONFIGURACOES----------------------------------------------------------------\n\n");
    printf("Politica de escrita: %s\n", c->politica_write == 1 ? "Write-back" : "Write-through");
    printf("Tamanho da linha: %d bytes\n", c->tam_linha);
    printf("Associatividade: %d\n", c->vias_associacao);
    printf("Politica de substituicao: %s\n", c->politica_subs == 1 ? "LRU" : "Random");
    printf("Tempo de hit: %d ns\n", header.tempo_hit);
    printf("Tempo de leitura da memoria: %d ns\n", header.tempo_read_mem);
    printf("Tempo de escrita da memoria: %d ns\n", header.tempo_write_mem);
}

void print_resultado(Cache* c) {
    double read_hit_rate = header.cont_read_total ? ((double)header.hits_read / header.cont_read_total) : 0.0;
    double write_hit_rate = header.cont_write_total ? ((double)header.hits_write / header.cont_write_total) : 0.0;
    double total_accesses = (header.cont_read_total + header.cont_write_total);
    double hit_rate = total_accesses ? ((double)(header.hits_read + header.hits_write) / total_accesses) : 0.0;
    double miss_rate = 1.0 - hit_rate;

    double tempo_memoria = total_accesses ? 
        ((header.cont_mem_read * header.tempo_read_mem + header.cont_mem_write * header.tempo_write_mem) / total_accesses) : 0.0;

    double tempo_medio = hit_rate * header.tempo_hit + miss_rate * tempo_memoria;

    printf("\n--RESULTADOS-------------------------------------------------------------------\n\n");
    printf("Leituras: %d\n", header.cont_read_total);
    printf("Escritas: %d\n", header.cont_write_total);
    printf("Leituras memoria principal: %d\n", header.cont_mem_read);
    printf("Escritas memoria principal: %d\n", header.cont_mem_write);
    printf("Hit rate de leitura: %.1f%%\n", read_hit_rate * 100);
    printf("Hit rate de escrita: %.1f%%\n", write_hit_rate * 100);
    printf("Hit rate de global: %.1f%%\n", hit_rate * 100);
    printf("Tempo medio de acesso em ns: %.4f ns\n", tempo_medio);
}

// ----------------------------- Main ----------------------------------------

int main() {
    setHeader();
    srand((unsigned int)time(NULL));

    Cache* cache = malloc(sizeof(Cache));
    if (!cache) {
        fprintf(stderr, "Erro! Falha ao alocar memoria para cache.\n");
        return EXIT_FAILURE;
    }

    // Cache configuration
    cache->politica_write = 1;       	// 0 = write through, 1 = write back
    cache->politica_subs = 1;        	// 0 = random, 1 = LRU
    cache->vias_associacao = 2;      	// associativity (lines per set)
    cache->tam_linha = 64;             	// line/block size in bytes
    cache->tamanho_total_cache = 8192;	// total cache size in bytes

    cache->nro_conjutos = cache->tamanho_total_cache / (cache->tam_linha * cache->vias_associacao);

    // Allocate sets and lines
    cache->conjuntos = malloc(sizeof(CacheConj) * cache->nro_conjutos);
    if (!cache->conjuntos) {
        fprintf(stderr, "Erro! Falha ao alocar memoria para conjuntos.\n");
        free(cache);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < cache->nro_conjutos; i++) {
        cache->conjuntos[i].linhas = calloc(cache->vias_associacao, sizeof(Linha));
        if (!cache->conjuntos[i].linhas) {
            fprintf(stderr, "Erro! Falha ao alocar memoria para linhas.\n");
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(cache->conjuntos[j].linhas);
            }
            free(cache->conjuntos);
            free(cache);
            return EXIT_FAILURE;
        }
    }

    // Timing parameters
    header.tempo_read_mem = 70;
    header.tempo_write_mem = 70;
    header.tempo_hit = 5;

    // Open input file
    FILE* f = fopen("oficial.cache", "r");
    if (!f) {
        fprintf(stderr, "Erro ao abrir arquivo 'oficial.cache'.\n");
        // Free allocated memory before exiting
        for (int i = 0; i < cache->nro_conjutos; i++) {
            free(cache->conjuntos[i].linhas);
        }
        free(cache->conjuntos);
        free(cache);
        return EXIT_FAILURE;
    }

    char linha[TAM_LINHA];
    while (fgets(linha, sizeof(linha), f)) {
        unsigned int endereco;
        char op;

        if (sscanf(linha, "%x %c", &endereco, &op) == 2) {
            simula_cache(cache, endereco, op);
        }
    }
    fclose(f);

    // Final write-back flush if needed
    if (cache->politica_write == 1) {
        varredura_final(cache);
    }

    print_config(cache);
    print_resultado(cache);

    // Free memory
    for (int i = 0; i < cache->nro_conjutos; i++) {
        free(cache->conjuntos[i].linhas);
    }
    free(cache->conjuntos);
    free(cache);

    return EXIT_SUCCESS;
}

