#include <mpi.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <vector>

uint8_t  aplicarFiltroGris(const uint8_t* A, int width, int x, int y){
    int rgba = 3;
    int fila = (y) * width * rgba;
    int columna = (x) * rgba;
    int indice = fila + columna;
    int valor = 0.21*A[indice]+0.72*A[indice+1]+0.07*A[indice+2];
    return  (uint8_t) valor;
}

int main( int argc, char *argv[])
{
    int rank, nprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    uint8_t* rgb_pixels = nullptr;
    uint8_t* rgb_pixels_local = nullptr;

    uint8_t* grey_pixels = nullptr;
    uint8_t* grey_pixels_local = nullptr;

    int real_size;
    int n;
    int block_size;
    int padding = 0;

    int width, height, channels;

    if(rank==0){
        rgb_pixels =
                stbi_load("image01.jpg", &width, &height, &channels, STBI_rgb);

        n = width*height*3;
        real_size = n;
        block_size = n / nprocs;

        if(n % nprocs !=0){
            real_size = std::ceil((double)n/nprocs)*nprocs;
            block_size = real_size/nprocs;
            padding = real_size-n;
        }
    }

    MPI_Bcast(block_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(padding, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(height, 1, MPI_INT, 0, MPI_COMM_WORLD);

    rgb_pixels_local =(uint8_t*)malloc(block_size* sizeof(uint8_t));
    grey_pixels_local =(uint8_t*)malloc(block_size/3* sizeof(uint8_t));

    MPI_Scatter(rgb_pixels, block_size, MPI_INT, rgb_pixels_local, block_size, MPI_INT, 0 ,MPI_COMM_WORLD);

    int new_block_size = block_size;
    if(rank == nprocs-1) new_block_size = block_size-padding;

    for(int i = 0; i < height; i++){
        for(int k = 0; k<width; k++){
            //fila+columna+componente
            int fila = (i*width); //grupos de to do el ancho
            int columna = (k);
            auto pixelBlur = aplicarFiltroGris(rgb_pixels_local,new_block_size, k, i);
            grey_pixels_local[fila+columna] = pixelBlur;
        }
    }

    MPI_Gather(grey_pixels_local, block_size, MPI_INT, grey_pixels, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Finalize();

    return 0;
}