#include <bits/stdc++.h>
using namespace std;

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

enum BI_COMPRESS_TYPE { BI_RGB = 0, BI_RLE8 = 1, BI_RLE4 = 2 };

struct BITMAPFILEHEADER {
    WORD Type;
    DWORD Size;
    WORD Reserved;
    WORD Reserved2;
    DWORD OffsetBits;
} __attribute__((packed));

struct BITMAPINFOHEADER {
    DWORD biSize;
    DWORD biWidth;
    DWORD biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    DWORD biXPelsPerMeter;
    DWORD biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} __attribute__((packed));

struct RGBTRI {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
} __attribute__((packed));

struct RGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} __attribute__((packed));

int log2int(int x)
{
    int ret = -1;
    while(x)
        x >>= 1, ret++;
    return ret;
}

struct BMPIMAGE {
    BITMAPFILEHEADER fileheader;
    BITMAPINFOHEADER infoheader;
    RGBQUAD *paleta;
    BYTE *dados;

    DWORD paletaSize, dataSize, numPixels, bytesPerLine;

    // construtor padr�o
    BMPIMAGE()
    {
        paleta = NULL;
        dados = NULL;
        paletaSize = dataSize = 0;
    }

    // construtor que cria uma c�pia de outra imagem
    BMPIMAGE(BMPIMAGE *img)
    {
        fileheader = img->fileheader;
        infoheader = img->infoheader;

        paletaSize = img->paletaSize;
        dataSize = img->dataSize;
        numPixels = img->numPixels;
        bytesPerLine = img->bytesPerLine;

        DWORD numCores = paletaSize / sizeof(RGBQUAD);
        paleta = new RGBQUAD[numCores];
        dados = new BYTE[dataSize];

        memcpy(paleta, img->paleta, paletaSize);
        memcpy(dados, img->dados, dataSize);
    }

    ~BMPIMAGE()
    {
        clearBMP();
    }

    void clearPaleta()
    {
        if(paletaSize) delete[] paleta;
        paleta = NULL;
    }

    void clearDados()
    {
        if(dataSize) delete[] dados;
        dados = NULL;
    }

    void clearBMP()
    {
        // libera a mem�ria da paleta e dados
        clearPaleta();
        clearDados();
    }

    int readBMP(char *filename)
    {
        FILE *file = fopen(filename, "rb");

        if(!file) {
            return 0;
        }

        clearBMP();

        fprintf(stderr, "lendo BMP %s\n", filename);

        // l� o header do arquivo imagem
        fread(&fileheader, 1, sizeof(BITMAPFILEHEADER), file);
        fread(&infoheader, 1, sizeof(BITMAPINFOHEADER), file);

        // verifica se formato da imagem � aceit�vel (deve ser �BM�)
        if (fileheader.Type != (('M' << 8) | 'B'))
            return 0;

        // o tamanho da paleta � o offsetBits (come�o dos dados da imagem) menos o tamanho dos dois headers
        paletaSize = fileheader.OffsetBits - (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

        fprintf(stderr, "calculando tamanho da paleta = %d\n", paletaSize);

        // se existir paleta, realizar sua leitura
        if(paletaSize > 0)
        {
            DWORD numCores = paletaSize / sizeof(RGBQUAD);
            paleta = new RGBQUAD[numCores];

            if(paleta == NULL) {
                return 0;
            }

            fread(paleta, 1, paletaSize, file);

            fprintf(stderr, "leitura paleta ok!\n");
        }

        // determina o tamanho da �rea de dados para leitura
        dataSize = fileheader.Size - fileheader.OffsetBits;

        // determina o n�mero de pixels na �rea de dados como o produto entre altura e largura
        numPixels = infoheader.biWidth * infoheader.biHeight;

        bytesPerLine = (infoheader.biWidth * infoheader.biBitCount + 31) / 32 * 4;

        fprintf(stderr, "lendo datasize = %d\n", dataSize);

        // aloca e l� os dados
        dados = new BYTE[dataSize];
        if(dados == NULL) {
            return 0;
        }

        fread(dados, 1, dataSize, file);
        fclose(file);

        fprintf(stderr, "leitura ok!\n");

        return 1;
    }

    int writeBMP(char *filename)
    {
        if(dataSize == 0) {
            fprintf(stderr, "tentando escrever BMP %s invalido\n", filename);
            return 0;
        }

        FILE *file = fopen(filename, "wb");

        fprintf(stderr, "escrevendo BMP %s\n", filename);

        if(!file) {
            return 0;
        }

        // escreve os cabe�alhos
        fwrite(&fileheader, 1, sizeof(BITMAPFILEHEADER), file);
        fwrite(&infoheader, 1, sizeof(BITMAPINFOHEADER), file);

        // se houver paleta, escreve a paleta
        if(paletaSize > 0) {
            fprintf(stderr, "escrevendo paleta %d\n", paletaSize);
            fwrite(paleta, 1, paletaSize, file);
        }

        // escreve os dados
        fwrite(dados, 1, dataSize, file);
        fclose(file);

        fprintf(stderr, "escrita ok!\n");

        return 1;
    }

    int alocaPaleta(int numCores, RGBQUAD *novaPaleta, BYTE *novoDados)
    {
        fprintf(stderr, "alocando paleta %d cores\n", numCores);

        // libera a mem�ria anterior
        clearBMP();

        // calcula o novo tamanho, em bytes, da paleta e dos dados
        paletaSize = numCores * sizeof(RGBQUAD);
        infoheader.biBitCount = log2int(numCores);
        bytesPerLine = (infoheader.biWidth * infoheader.biBitCount + 31) / 32 * 4;
        dataSize = infoheader.biHeight * bytesPerLine;

        // aloca nova paleta e dados e copia conte�do dos par�metros passados
        paleta = new RGBQUAD[numCores];
        dados = new BYTE[dataSize];

        if(!paleta || !dados) {
            return 0;
        }

        memcpy(paleta, novaPaleta, paletaSize);
        memcpy(dados, novoDados, dataSize);

        // por fim, calcula o novo offsetBits e o novo tamanho do arquivo
        fileheader.OffsetBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + paletaSize;
        fileheader.Size = fileheader.OffsetBits + dataSize;

        fprintf(stderr, "alocando ok!\n");

        return 1;
    }

    int aplicaRLE(BI_COMPRESS_TYPE biCompression, BYTE *novoDados, int novoDataSize)
    {
        fprintf(stderr, "aplicando RLE %d bytes\n", novoDataSize);

        // libera os dados anteriores
        clearDados();

        // aloca novo dados
        dados = new BYTE[novoDataSize];

        if(!dados) {
            return 0;
        }

        // copia conte�do dos dados passados por par�metro
        memcpy(dados, novoDados, novoDataSize);

        // atualiza dataSize e as v�riaveis de tamanho do arquivo e compress�o nos headers
        dataSize = novoDataSize;
        fileheader.Size = fileheader.OffsetBits + dataSize;
        infoheader.biCompression = (DWORD)biCompression;
        infoheader.biSizeImage = dataSize;

        fprintf(stderr, "aplicando RLE ok!\n");

        return 1;
    }
};

void BMP_quantizacaoUniforme_8Bits(BMPIMAGE *img)
{
    if(img->infoheader.biBitCount != 24) return;

    // calcula o n�mero de bytes por linha e o n�mero de bytes da �rea de dados para a imagem ap�s inser��o da paleta
    DWORD bytesPerLine = (img->infoheader.biWidth * 8 + 31) / 32 * 4;
    DWORD dataSize = img->infoheader.biHeight * bytesPerLine;

    RGBQUAD *paleta = new RGBQUAD[256];
    BYTE *dados = new BYTE[dataSize];

    if(!paleta || !dados) {
        return;
    }

    // preenche a paleta com cores da quantiza��o uniforme
    // as 256 cores da paleta tem 8 tons de vermelho, 8 tons de verde e 4 tons de azul (8*8*4 = 256),
    // distribu�dos uniformemente pelos 256 tons poss�veis
    int ind = 0;

    for(int R = 16; R < 256; R += 32)
        for(int G = 16; G < 256; G += 32)
            for(int B = 32; B < 256; B += 64)
            {
                paleta[ind].rgbRed = R;
                paleta[ind].rgbGreen = G;
                paleta[ind].rgbBlue = B;
                paleta[ind].rgbReserved = 0;
                ind++;
            }

    assert(ind == 256);

    ind = 0;

    // preenche a �rea de dados calculando, para cada cor, o �ndice da sua cor mais pr�xima na paleta

    // o la�o externo itera pelas linhas da imagem, incrementando 'offset_lin' pelo n�mero de bytes por linha
    for(DWORD i = 0, offset_lin = 0; i < img->infoheader.biHeight; i++, offset_lin += img->bytesPerLine)
    {
        // o la�o interno itera pelas colunas da imagem, incrementando 'offset_col' pelo tamanho de RGBTRI (3 em 3)
        for(DWORD j = 0, offset_col = 0; j < img->infoheader.biWidth; j++, offset_col += sizeof(RGBTRI))
        {
            // joga o pixel na posi��o do vetor de dados para uma vari�vel de pixel (tipo RGBTRI)
            RGBTRI *pixel = (RGBTRI*) &img->dados[offset_lin + offset_col];

            // calcula a cor mais pr�xima dessa cor na quantiza��o uniforme
            pixel->rgbBlue  = (pixel->rgbBlue  / 64) * 64 + 32;
            pixel->rgbGreen = (pixel->rgbGreen / 32) * 32 + 16;
            pixel->rgbRed   = (pixel->rgbRed   / 32) * 32 + 16;

            // utiliza a cor calculada para, marotamente, encontrar o �ndice na paleta dessa cor
            // o porqu� disso funcionar fica como um exerc�cio para o leitor (leitor, no caso, voc�, Adair huehuebrbr)
            dados[ind++] = (pixel->rgbRed / 32) * 32 + (pixel->rgbGreen / 32) * 4 + (pixel->rgbBlue / 64);
        }

        // preenche os bytes de padding da nova imagem (lembrando que bytesPerLine � o n�mero
        //   de bytes por linha na imagem ap�s inser��o da paleta)
        for(DWORD j = img->infoheader.biWidth; j < bytesPerLine; j++)
            dados[ind++] = 0;
    }

    assert((DWORD)ind == dataSize);

    img->alocaPaleta(256, paleta, dados);

    delete[] paleta;
    delete[] dados;
}

// fun��o que comprime a imagem em RLE utilizando apenas modo encoded
// (ou seja, n�o inclui sequ�ncias do tipo 0 X [...] indicando X bytes diferentes n�o comprimidos)
void BMP_RLEencoding_encoded_only(BMPIMAGE *img)
{
    if(img->infoheader.biBitCount != 8) return;

    // calcula o maior tamanho poss�vel para a imagem (cada pixel pode virar 2 pixels + 2 para o fim de linha) * n�mero de linhas + 2 para fim de imagem
    DWORD maxDataSize = (img->infoheader.biWidth * 2 + 2) * img->infoheader.biHeight + 2;
    BYTE *dados = new BYTE[maxDataSize];

    if(!dados) {
        return;
    }

    int ind = 0;

    // itera linha a linha
    for(DWORD i = 0, offset_lin = 0; i < img->infoheader.biHeight; i++, offset_lin += img->bytesPerLine)
    {
        int cont = 1;

        DWORD j;
        // itera coluna a coluna
        for(j = 1; j < img->infoheader.biWidth; j++)
        {
            // se este pixel for igual ao anterior, estamos continuando uma sequ�ncia de bytes iguais
            if(img->dados[offset_lin + j] == img->dados[offset_lin + j - 1])
            {
                // se cont == 255, temos que escrever a sequ�ncia atual pois o valor m�ximo do contador � 255 (1 byte)
                if(cont == 255)
                {
                    dados[ind++] = cont;
                    dados[ind++] = img->dados[offset_lin + j];
                    cont = 0;
                }
                cont++;
            }
            else
            {
                // sen�o, encontramos dois bytes diferentes; escrevemos a sequ�ncia que acabou de se encerrar
                dados[ind++] = cont;
                dados[ind++] = img->dados[offset_lin + j - 1];
                cont = 1;
            }
        }

        // escreve a �ltima sequ�ncia da linha
        dados[ind++] = cont;
        dados[ind++] = img->dados[offset_lin + j - 1];

        // escreve sequ�ncia de fim de linha
        dados[ind++] = 0;
        dados[ind++] = 0;
    }

    // escreve sequ�ncia de fim de imagem
    dados[ind++] = 0;
    dados[ind++] = 1;

    assert((DWORD)ind <= maxDataSize);

    img->aplicaRLE(BI_RLE8, dados, ind);

    delete[] dados;
}

// fun��o auxiliar para n�o ter que ficar reescrevendo esse trecho
void __BMP_RLEencoding_escreve_dif(BMPIMAGE *img, BYTE *dados, int cont_dif, int ultimo_byte, int &ind)
{
    // se o n�mero de bytes na sequ�ncia de bytes diferentes for maior que 2, imprimimos 0 X e os bytes sem compress�o
    // caso contr�rio, temos que imprimir 1 [byte1] 1 [byte2], se forem 2 diferentes, ou 1 [byte] se for 1 diferente,
    // pois n�o podemos imprimir 0 1 [byte1] ou 0 2 [byte1] [byte2], pois 0 1 e 0 2 s�o c�digos especiais

    if(cont_dif == 1)
    {
        dados[ind++] = 1;
        dados[ind++] = img->dados[ultimo_byte];
    }
    else if(cont_dif == 2)
    {
        dados[ind++] = 1;
        dados[ind++] = img->dados[ultimo_byte - 1];
        dados[ind++] = 1;
        dados[ind++] = img->dados[ultimo_byte];
    }
    else
    {
        dados[ind++] = 0;
        dados[ind++] = cont_dif;
        for(int k = 0; k < cont_dif; k++)
            dados[ind++] = img->dados[ultimo_byte + 1 - cont_dif + k];
        if(cont_dif % 2 == 1)
            dados[ind++] = 0;   // n�o est� na especifica��o, mas combinamos com o pessoal do irfanview que a gente ia colocar esse zero aqui
    }
}

void __BMP_RLEencoding_escreve_iguais(BMPIMAGE *img, BYTE *dados, int cont_iguais, int ultimo_byte, int &ind)
{
    dados[ind++] = cont_iguais;
    dados[ind++] = img->dados[ultimo_byte];
}

// fun��o que comprime a imagem em RLE usando tanto modo encoded como modo absolute
void BMP_RLEencoding(BMPIMAGE *img)
{
    if(img->infoheader.biBitCount != 8) return;

    DWORD maxDataSize = (img->infoheader.biWidth * 2 + 2) * img->infoheader.biHeight + 2;
    BYTE *dados = new BYTE[maxDataSize];

    if(!dados) {
        return;
    }

    int ind = 0;

    // itera linha a linha
    for(DWORD i = 0, offset_lin = 0; i < img->infoheader.biHeight; i++, offset_lin += img->bytesPerLine)
    {
        DWORD j;
        int cont_iguais = 1, cont_dif = 1;

        // itera coluna a coluna mantendo um contador de bytes iguais e bytes diferentes
        for(j = 1; j < img->infoheader.biWidth; j++)
        {
            if(img->dados[offset_lin + j] == img->dados[offset_lin + j - 1])
            {
                // encontramos dois bytes iguais, ent�o se havia um sequ�ncia de bytes diferentes, escrev�-la
                cont_dif--;
                if(cont_dif > 0) {
                    // repare que o pen�ltimo par�metro (�ltimo byte da sequ�ncia a ser escrita) � o byte atual menos dois, pois se
                    // entramos nesse if � porque o byte atual � igual ao byte anterior, ent�o ambos n�o entram na sequ�ncia de diferentes
                    __BMP_RLEencoding_escreve_dif(img, dados, cont_dif, offset_lin + j - 2, ind);
                }
                cont_dif = 0;

                // se cont == 255, temos que escrever a sequ�ncia atual pois o valor m�ximo do contador � 255 (1 byte)
                if(cont_iguais == 255)
                {
                    __BMP_RLEencoding_escreve_iguais(img, dados, cont_iguais, offset_lin + j - 1, ind);
                    cont_iguais = 0;
                }
                cont_iguais++;
            }
            else
            {
                // idem acima para os dois coment�rios

                if(cont_iguais > 1) {
                    __BMP_RLEencoding_escreve_iguais(img, dados, cont_iguais, offset_lin + j - 1, ind);
                }
                cont_iguais = 1;

                if(cont_dif == 255) {
                    __BMP_RLEencoding_escreve_dif(img, dados, cont_dif, offset_lin + j - 1, ind);
                    cont_dif = 0;
                }
                cont_dif++;
            }
        }

        // escreve a �ltima sequ�ncia da linha
        if(cont_iguais > 1)
        {
            __BMP_RLEencoding_escreve_iguais(img, dados, cont_iguais, offset_lin + j - 1, ind);
        }
        else
        {
            __BMP_RLEencoding_escreve_dif(img, dados, cont_dif, offset_lin + j - 1, ind);
        }

        // escreve sequ�ncia de fim de linha
        dados[ind++] = 0;
        dados[ind++] = 0;
    }

    // escreve sequ�ncia de fim de imagem
    dados[ind++] = 0;
    dados[ind++] = 1;

    assert((DWORD)ind < maxDataSize);

    img->aplicaRLE(BI_RLE8, dados, ind);

    delete[] dados;
}

int main(int argc, char **argv)
{
    /*
    * Etcha GCC, n�o me quebra as perna por favor - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52991
    * No Windows, compilar com -mno-ms-bitfields parece fazer o __attribute__ ((packed)) funcionar.
    - No Linux??
    */

    assert(sizeof(BITMAPFILEHEADER) == 14);
    assert(sizeof(BITMAPINFOHEADER) == 40);
    assert(sizeof(RGBTRI) == 3);
    assert(sizeof(RGBQUAD) == 4);

    char filename1[100], filename2[100], filename3[100], filename4[100];

    if(argc == 1)
    {
        printf("Nome do arquivo a alterar: ");
        scanf("%s", filename1);
    }
    else
    {
        strcpy(filename1, argv[1]);
    }

    BMPIMAGE img;

    img.readBMP(filename1);

    filename1[strlen(filename1) - 4] = 0;
    sprintf(filename2, "%s-quantizado.bmp", filename1);
    sprintf(filename3, "%s-quantizado-RLE-encoded.bmp", filename1);
    sprintf(filename4, "%s-quantizado-RLE-completo.bmp", filename1);

    BMP_quantizacaoUniforme_8Bits(&img);
    img.writeBMP(filename2);

    BMPIMAGE img2(&img);

    BMP_RLEencoding_encoded_only(&img2);
    img2.writeBMP(filename3);

    BMP_RLEencoding(&img);
    img.writeBMP(filename4);

    return 0;
}
