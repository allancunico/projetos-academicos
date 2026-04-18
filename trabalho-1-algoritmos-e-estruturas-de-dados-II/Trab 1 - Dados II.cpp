#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PRODUTOS 5000
#define MAX_PEDIDOS MAX_PRODUTOS
#define SALTO_IDX 100

typedef struct {
    unsigned long long produto_id;	// chave unica
    unsigned long long categoria_id;
    char produto_genero;	// m ou f
} Produto;

typedef struct {
    char order_datetime[25];	// (2018-12-06 13:33:09 UTC)
    unsigned long long pedido_id;	// chave unica
    unsigned long long produto_id;
    double preco_usd;
    unsigned long long user_id;
} Pedido;

typedef struct {
    unsigned long long chave;
    long posicao;
} Indice;

const char produto_bin[] = "produtos.bin";
const char pedido_bin[] = "pedidos.bin";
const char produto_idx[] = "produtos.idx";
const char pedido_idx[] = "pedidos.idx";

int chaves[MAX_PRODUTOS];
int pedido_cont = 0;
int produto_cont = 0;

//======================================================================================================================================

int compara_order_id(const void *a, const void *b) {
    const Pedido *pa = (const Pedido *)a;
    const Pedido *pb = (const Pedido *)b;

    if (pa->pedido_id < pb->pedido_id) return -1;
    if (pa->pedido_id > pb->pedido_id) return 1;
    return 0;
}

void ordena_pedidos_por_chave(const char *file) {
    FILE *bin = fopen(file, "rb+");
    if (!bin) { 
        perror("Erro ao abrir arquivo binario para ordenacao"); 
        exit(EXIT_FAILURE); 
    }

    fseek(bin, 0, SEEK_END);
    long total_bytes = ftell(bin);
    int total_pedidos = total_bytes / sizeof(Pedido);
    rewind(bin);

    Pedido *buffer = (Pedido *) malloc(total_pedidos * sizeof(Pedido));
    if (!buffer) { 
        fclose(bin); 
        perror("malloc falhou"); 
        return; 
    }

    fread(buffer, sizeof(Pedido), total_pedidos, bin);

    qsort(buffer, total_pedidos, sizeof(Pedido), compara_order_id);

    rewind(bin);
    fwrite(buffer, sizeof(Pedido), total_pedidos, bin);

    fclose(bin);
    free(buffer);

    printf("Arquivo de pedidos ordenado\n");
}

int compara_produto_id(const void *a, const void *b) {
    const Produto *pa = (const Produto *)a;
    const Produto *pb = (const Produto *)b;

    if (pa->produto_id < pb->produto_id) return -1;
    if (pa->produto_id > pb->produto_id) return 1;
    return 0;
}

void ordena_produtos_por_chave(const char *file) {
    FILE *bin = fopen(file, "rb+");
    if (!bin) { perror("Erro ao abrir arquivo binario ordenacao"); exit(EXIT_FAILURE); }

    fseek(bin, 0, SEEK_END);
    long total_bytes = ftell(bin);
    int total_produtos = total_bytes / sizeof(Produto);
    rewind(bin);

    Produto *buffer = (Produto *) malloc(total_produtos * sizeof(Produto));
    if (!buffer) { fclose(bin); perror("malloc falhou"); return; }

    fread(buffer, sizeof(Produto), total_produtos, bin);
    qsort(buffer, total_produtos, sizeof(Produto), compara_produto_id);

    rewind(bin);
    fwrite(buffer, sizeof(Produto), total_produtos, bin);
    fclose(bin);
    free(buffer);

    printf("Arquivo de produtos ordenado\n", total_produtos);
}

void cria_indice_produto(const char *arquivo_dados, const char *arquivo_indice, int salto) {
    FILE *fp_dados = fopen(arquivo_dados, "rb");
    FILE *fp_indice = fopen(arquivo_indice, "wb");
    if (!fp_dados || !fp_indice) {
        perror("Erro ao abrir arquivo");
        return;
    }

    Indice idx;
    Produto p;
    long pos;
    int cont = 0;

    while (fread(&p, sizeof(Produto), 1, fp_dados) == 1) {
        if (cont % salto == 0) {
            pos = ftell(fp_dados) - sizeof(Produto);
            idx.chave = p.produto_id;
            idx.posicao = pos;
            fwrite(&idx, sizeof(Indice), 1, fp_indice);
        }
        cont++;
    }

    fclose(fp_dados);
    fclose(fp_indice);
    printf("Indice criado com 1 a cada %d registros.\n", salto);
}

void cria_indice_pedido(const char *arquivo_dados, const char *arquivo_indice, int salto) {
    FILE *fp_dados = fopen(arquivo_dados, "rb");
    FILE *fp_indice = fopen(arquivo_indice, "wb");
    if (!fp_dados || !fp_indice) {
        perror("Erro ao abrir arquivo");
        if (fp_dados) fclose(fp_dados);
        if (fp_indice) fclose(fp_indice);
        return;
    }

    Pedido p;
    Indice idx;
    long pos;
    int cont = 0;

    while (fread(&p, sizeof(Pedido), 1, fp_dados) == 1) {
        if (cont % salto == 0) {
            pos = ftell(fp_dados) - sizeof(Pedido);
            idx.chave = p.pedido_id;
            idx.posicao = pos;
            fwrite(&idx, sizeof(Indice), 1, fp_indice);
        }
        cont++;
    }

    fclose(fp_dados);
    fclose(fp_indice);
    printf("Indice de pedidos criado com 1 entrada a cada %d registros.\n", salto);
}

int prod_existe(unsigned long long produto_id) {
    for (int i = 0; i < produto_cont; i++) {
        if (chaves[i] == produto_id)
            return 1;
    }
    return 0;
}

int parse(const char *linha, Pedido *ped, Produto *prod) {
    char buffer[512];
    strncpy(buffer, linha, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char *token = strtok(buffer, ",");
    int col = 0;
    char genero = ' ';
    unsigned long long categoria_id = 0;

    while (token) {
        col++;
        switch (col) {
            case 1: strncpy(ped->order_datetime, token, 24); ped->order_datetime[24] = '\0'; break;
            case 2: ped->pedido_id = strtoull(token, NULL, 10); break;
            case 3: ped->produto_id = strtoull(token, NULL, 10); break;
            case 5: categoria_id = strtoull(token, NULL, 10); break;
            case 8: ped->preco_usd = atof(token); break;
            case 9: ped->user_id = strtoull(token, NULL, 10); break;
            case 10: genero = token[0]; break;
        }
        token = strtok(NULL, ",");
    }

    if (ped->pedido_id && ped->produto_id && categoria_id && ped->user_id) {
        prod->produto_id = ped->produto_id;
        prod->categoria_id = categoria_id;
        if(genero=='f' || genero=='m'){
        	prod->produto_genero = genero;
		} else{
			prod->produto_genero = ' ';
		}
        return 1;
    }
    return 0;
}

void processa_dataset(const char *dataset_csv, const char *prod_bin, const char *ped_bin) {
    FILE *csv = fopen(dataset_csv, "r");
    FILE *fp_prod = fopen(prod_bin, "wb");
    FILE *fp_ped = fopen(ped_bin, "wb");
    if (!csv || !fp_prod || !fp_ped) {
        perror("Erro ao abrir arquivos");
        exit(1);
    }

    char linha[512];
    while (fgets(linha, sizeof(linha), csv)) {
        Produto prod;
        Pedido ped;
        memset(&prod, 0, sizeof(Produto));
        memset(&ped, 0, sizeof(Pedido));

        if (parse(linha, &ped, &prod)) {
            if (!prod_existe(prod.produto_id) && produto_cont < MAX_PRODUTOS) {
                fwrite(&prod, sizeof(Produto), 1, fp_prod);
                chaves[produto_cont++] = prod.produto_id;
            }
            if (pedido_cont < MAX_PEDIDOS) {
                fwrite(&ped, sizeof(Pedido), 1, fp_ped);
                pedido_cont++;
            }
        }
    }

    fclose(csv);
    fclose(fp_prod);
    fclose(fp_ped);
    printf("Arquivos binários criados com sucesso.\n");
}

//======================================================================================================================================

void total_vendas_produtos_por_genero(const char *produtos_bin, const char *pedidos_bin, const char genero) {
    FILE *fp_ped = fopen(pedidos_bin, "rb");
    FILE *fp_prod = fopen(produtos_bin, "rb");
    if (!fp_ped || !fp_prod) {
        perror("Erro ao abrir arquivo");
        return;
    }

    Pedido pedido;
    Produto produto;
    double total = 0.0;

    while (fread(&pedido, sizeof(Pedido), 1, fp_ped) == 1) {
        rewind(fp_prod);
        while (fread(&produto, sizeof(Produto), 1, fp_prod) == 1) {
            if (produto.produto_id == pedido.produto_id && produto.produto_genero == genero) {
                total += pedido.preco_usd;
                break;
            }
        }
    }

    printf("Total de vendas de produtos %s: $ %.2f\n", (genero == 'm' ? "masculinos" : "femininos"), total);

    fclose(fp_ped);
    fclose(fp_prod);
}

void total_vendas_por_produto(const char *produtos_bin, const char *pedidos_bin) {
    FILE *fp_prod = fopen(produtos_bin, "rb");
    if (!fp_prod) {
        perror("Erro ao abrir produtos.bin");
        return;
    }

    Produto produto;
    while (fread(&produto, sizeof(Produto), 1, fp_prod) == 1) {
        FILE *fp_ped = fopen(pedidos_bin, "rb");
        if (!fp_ped) {
            perror("Erro ao abrir pedidos.bin");
            fclose(fp_prod);
            return;
        }

        Pedido pedido;
        double total = 0.0;

        while (fread(&pedido, sizeof(Pedido), 1, fp_ped) == 1) {
            if (pedido.produto_id == produto.produto_id) {
                total += pedido.preco_usd;
            }
        }

        fclose(fp_ped);
        printf("Produto ID %llu: Total de vendas = $%.2f\n", produto.produto_id, total);
    }

    fclose(fp_prod);
}

void produto_mais_vendido(const char *produtos_bin, const char *pedidos_bin) {
    FILE *fp_prod = fopen(produtos_bin, "rb");
    if (!fp_prod) {
        perror("Erro ao abrir produtos.bin");
        return;
    }

    Produto produto;
    unsigned long long id_mais_vendido = 0;
    double maior_total = 0.0;

    while (fread(&produto, sizeof(Produto), 1, fp_prod) == 1) {
        FILE *fp_ped = fopen(pedidos_bin, "rb");
        if (!fp_ped) {
            perror("Erro ao abrir pedidos.bin");
            fclose(fp_prod);
            return;
        }

        Pedido pedido;
        double total = 0.0;

        while (fread(&pedido, sizeof(Pedido), 1, fp_ped) == 1) {
            if (pedido.produto_id == produto.produto_id) {
                total += pedido.preco_usd;
            }
        }

        fclose(fp_ped);

        if (total > maior_total) {
            maior_total = total;
            id_mais_vendido = produto.produto_id;
        }
    }

    fclose(fp_prod);

    if (id_mais_vendido != 0) {
        printf("Produto mais vendido: ID %llu, com $%.2f em vendas\n",
               id_mais_vendido, maior_total);
    } else {
        printf("Nenhum produto encontrado.\n");
    }
}

void escreve_todos_os_dados(const char *produtos_bin, const char *pedidos_bin, int opt) {
    FILE *fp_prod = fopen(produtos_bin, "rb");
    FILE *fp_ped = fopen(pedidos_bin, "rb");
    if (!fp_prod || !fp_ped) {
        perror("Erro ao abrir arquivo");
        return;
    }

    printf("--------------------------------------------------------------------------------------------\n");
    printf("Dados do arquivo de %s:\n", (opt == 1 ? "produtos" : "pedidos"));

    int cont = 1;

    if (opt == 1) {
        Produto produto;
        printf("       %-19s, %-19s, %-15s\n", "Product ID", "Category ID", "Product gender");

        while (fread(&produto, sizeof(Produto), 1, fp_prod) == 1) {
            printf("#%-3d - %llu, %llu, %c\n",
                   cont++, produto.produto_id, produto.categoria_id, produto.produto_genero);
        }
    } else {
        Pedido pedido;
        printf("       %-19s, %-23s, %-19s, %-9s, %-11s\n",
               "Order ID", "Order datetime", "Product ID", "Price USD", "User ID");

        while (fread(&pedido, sizeof(Pedido), 1, fp_ped) == 1) {
            printf("#%-3d - %llu, %s, %llu, $%-8.2f, %llu\n",
                   cont++, pedido.pedido_id, pedido.order_datetime,
                   pedido.produto_id, pedido.preco_usd, pedido.user_id);
        }
    }

    fclose(fp_ped);
    fclose(fp_prod);
}

long busca_indice_binaria(const char *arquivo_indice, unsigned long long chave) {
	FILE *fp = fopen(arquivo_indice, "rb");
	if (!fp) {
		perror("Erro ao abrir índice");
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	long tamanho = ftell(fp);
	int n = tamanho / sizeof(Indice);
	rewind(fp);

	int ini = 0, fim = n - 1, meio;
	Indice idx;
	long pos = -1;
	unsigned long long ultima_chave_menor = 0;

	while (ini <= fim) {
		meio = (ini + fim) / 2;
		fseek(fp, meio * sizeof(Indice), SEEK_SET);
		fread(&idx, sizeof(Indice), 1, fp);

		if (idx.chave == chave) {
			pos = idx.posicao;
			fclose(fp);
			return pos;
		} else if (idx.chave < chave) {
			pos = idx.posicao;
			ultima_chave_menor = idx.chave;
			ini = meio + 1;
		} else {
			fim = meio - 1;
		}
	}

	fclose(fp);
	return pos;
}

void busca_produto_por_id(const char *arquivo_dados, const char *arquivo_indice, unsigned long long chave) {
    long pos = busca_indice_binaria(arquivo_indice, chave);
    if (pos == -1) {
        printf("Chave nao encontrada no indice.\n");
        return;
    }

    FILE *fp = fopen(arquivo_dados, "rb");
    if (!fp) {
        perror("Erro ao abrir dados");
        return;
    }

    fseek(fp, pos, SEEK_SET);

    Produto p;
    while (fread(&p, sizeof(Produto), 1, fp) == 1) {
        if (p.produto_id == chave) {
            printf("Produto encontrado:\n");
            printf("ID: %llu | Categoria: %llu | Genero: %c\n", p.produto_id, p.categoria_id, p.produto_genero);
            fclose(fp);
            return;
        } else if (p.produto_id > chave) {
            break;
        }
    }

    printf("Produto não encontrado nos dados.\n");
    fclose(fp);
}

void busca_pedido_por_order_id(const char *arquivo_dados, const char *arquivo_indice, unsigned long long chave) {
    long pos = busca_indice_binaria(arquivo_indice, chave);
    if (pos == -1) {
        printf("Chave nao encontrada no indice.\n");
        return;
    }

    FILE *fp = fopen(arquivo_dados, "rb");
    if (!fp) {
        perror("Erro ao abrir arquivo de pedidos");
        return;
    }

    fseek(fp, pos, SEEK_SET);

    Pedido p;
    while (fread(&p, sizeof(Pedido), 1, fp) == 1) {
        if (p.pedido_id == chave) {
            printf("\nPedido encontrado!\n");
            printf("Order ID: %llu\n", p.pedido_id);
            printf("Data: %s\n", p.order_datetime);
            printf("Produto ID: %llu\n", p.produto_id);
            printf("Preço: %.2f USD\n", p.preco_usd);
            printf("User ID: %llu\n", p.user_id);
            fclose(fp);
            return;
        }
        if (p.pedido_id > chave) break;
    }

    printf("Pedido não encontrado nos dados.\n");
    fclose(fp);
}

void adiciona_produto(Produto novo, const char *arquivo_bin, const char *arquivo_idx, int salto) {

    if (prod_existe(novo.produto_id)) {
        printf("Produto ID %llu ja existe.\n", novo.produto_id);
        return;
    }


    FILE *fp = fopen(arquivo_bin, "ab");
    if (!fp) {
        perror("Erro ao abrir produtos.bin para adicionar");
        return;
    }

    fwrite(&novo, sizeof(Produto), 1, fp);
    fclose(fp);

    if (produto_cont < MAX_PRODUTOS) {
        chaves[produto_cont++] = novo.produto_id;
    } else {
        printf("Atingido limite maximo de produtos.\n");
    }

    ordena_produtos_por_chave(arquivo_bin);

    cria_indice_produto(arquivo_bin, arquivo_idx, salto);

    printf("Produto ID %llu adicionado com sucesso.\n", novo.produto_id);
}

void remove_produto(long pos_remover, const char *arquivo_bin, const char *arquivo_idx, int salto) {
    FILE *fp = fopen(arquivo_bin, "rb");
    if (!fp) {
        perror("Erro ao abrir produtos.bin");
        return;
    }

    Produto buffer[MAX_PRODUTOS];
    size_t total = fread(buffer, sizeof(Produto), MAX_PRODUTOS, fp);
    fclose(fp);

    if (pos_remover < 1 || pos_remover > total) {
        printf("Posição invalida.\n");
        return;
    }

    unsigned long long chave_removida = buffer[pos_remover - 1].produto_id;


    int idx_chave = -1;
    for (int i = 0; i < produto_cont; i++) {
        if (chaves[i] == chave_removida) {
            idx_chave = i;
            break;
        }
    }
    if (idx_chave != -1) {
        for (int i = idx_chave; i < produto_cont - 1; i++) {
            chaves[i] = chaves[i + 1];
        }
        produto_cont--;
    }

    fp = fopen(arquivo_bin, "wb");

    for (size_t i = 0; i < total; i++) {
        if ((long)i != pos_remover - 1) {
            fwrite(&buffer[i], sizeof(Produto), 1, fp);
        }
    }
    fclose(fp);

    ordena_produtos_por_chave(arquivo_bin);
    cria_indice_produto(arquivo_bin, arquivo_idx, salto);

    printf("Produto na posiçao #%ld removido com sucesso.\n", pos_remover);
}

//======================================================================================================================================

void inicializa() {
	processa_dataset("dataset.csv", produto_bin, pedido_bin);
    ordena_produtos_por_chave(produto_bin);
    ordena_pedidos_por_chave(pedido_bin);
    cria_indice_produto(produto_bin, produto_idx, SALTO_IDX);
    cria_indice_pedido(pedido_bin, pedido_idx, SALTO_IDX);    
}

int main() {
    //inicializa();	//e reseta os arquivos

    int opt, genero;
    long pos;
    unsigned long long busca;

    while (1) {
        printf("--------------------------------------------------------------------------------------------\n");
        printf("Escolha uma operacao:\n");
        printf("--------------------------------------------------------------------------------------------\n");
        printf(" 1 - Listar valor total de vendas por genero\n");
        printf(" 2 - Lista total de vendas por produto\n");
        printf(" 3 - Produto mais vendido\n");
        printf(" 4 - Mostrar dados\n");
        printf(" 5 - Consulta binaria de produto (Produto ID)\n");
        printf(" 6 - Consulta binaria de pedido (Pedido ID)\n");
        printf(" 7 - Adicionar um novo registro de produto\n");
        printf(" 8 - Remover um registro de produto\n");
        printf(" 0 - Sair e salvar\n");
        printf("--------------------------------------------------------------------------------------------\n");

        scanf("%d", &opt);

        switch (opt) {
            case 1:
                printf("Escolha por qual genero filtrar\n");
                printf(" 1 - Feminino\n 2 - Masculino\n");
                scanf("%d", &genero);
                total_vendas_produtos_por_genero(produto_bin, pedido_bin, (genero == 1 ? 'f' : 'm'));
                getchar(); getchar();
                printf("\n");
                break;

            case 2:
                total_vendas_por_produto(produto_bin, pedido_bin);
                getchar(); getchar();
                printf("\n");
                break;

            case 3:
                produto_mais_vendido(produto_bin, pedido_bin);
                getchar(); getchar();
                printf("\n");
                break;

            case 4:
                printf("Escolha qual dataset deseja listar\n");
                printf(" 1 - Produtos\n 2 - Pedidos\n");
                scanf("%d", &opt);

                if (opt == 1 || opt == 2) {
                    escreve_todos_os_dados(produto_bin, pedido_bin, opt);
                } else {
                    printf("Erro! opcao invalida.\n");
                }
                getchar(); getchar();
                break;
			
			case 5:
				printf("Digite o ID do produto que deseja buscar (numero de 19 casas)\n");	
				scanf("%llu", &busca);
			
				busca_produto_por_id(produto_bin, produto_idx, busca);
				getchar(); getchar();
				break;

			case 6:
				printf("Digite o ID do pedido que deseja buscar (numero de 19 casas)\n");
				scanf("%llu", &busca);
			
				busca_pedido_por_order_id(pedido_bin, pedido_idx, busca);
				getchar(); getchar();
				break;
					
			case 7:
				Produto novo;
				printf("Informe o ID do produto (19 digitos): ");
				scanf("%llu", &novo.produto_id);
				printf("Informe o ID da categoria do produto (19 digitos): ");
				scanf("%llu", &novo.categoria_id);
				printf("Informe o genero do produto (m - masculino, f - feminino): ");
				scanf(" %c", &novo.produto_genero);					
					
				adiciona_produto(novo, produto_bin, produto_idx, SALTO_IDX);
				break;	
				
			case 8:
				escreve_todos_os_dados(produto_bin, pedido_bin, 1);
    			printf("Digite a # posição do registro que deseja remover:\n");
    			scanf("%ld", &pos);
  				
				remove_produto(pos, produto_bin, produto_idx, 2);
    			getchar(); getchar();
    			break;
						
            case 0:
                printf("Saindo...\n");
                exit(0);
                break;
        }
    }

    return 0;
}
