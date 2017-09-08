//  main.cpp
//  lectura-scd
//
//  Created by Enrique Lazcorreta Puigmartí on 20/4/17.
//  Copyright $\copyright$ 2017 Enrique Lazcorreta Puigmartí. All rights reserved.
//

#include "../../lib/comun.h"
#include "../../lib/scd.h"

#include <iostream>
using std::cout;
using std::endl;
#include <iomanip>

#include <string>
using std::string;

#include <fstream>
using std::ifstream;
using std::ofstream;

#include <stdlib.h>


void Uso(const char * argv[])
{
    cout << "Uso:" << endl << endl;
    cout << '\t' << NombreAplicacion(argv[0]) << " dataset-de-clasificación "
            "atributo-a-eliminar [--verbose]" << endl << endl;
    cout << "Argumentos:" << endl << endl;
    cout << "\tdataset-de-clasificación es un archivo de texto plano separado "
            "por comas (formato CSV)." << endl << endl;
    cout << "\t                          Puede contener metadatos en las "
            " primeras líneas si su primer caracter es '@' o '#'." << endl << endl;
    cout << "\t                          La clase es el último dato de cada "
            "línea. Sólo si la primera línea del dataset contiene "
            "\"#clase_al_principio\" se tomará el primer dato como "
            "la clase del experimento." << endl << endl;
    cout << "\tatributo-a-eliminar es un número entre 1 y el número de "
            "atributos del dataset." << endl << endl;
    cout << "\t--verbose\t\t\tSi se añade este argumento se mostrará el proceso "
            "en la salida estándar." << endl << endl;
}



void ProcesaArgumentos(const int argc, const char * argv[], int *atributo_a_eliminar, bool *verbose)
{
    if (argc < 3 || argc > 4)
    {
        Uso(argv);
        exit(1);
    }
    
    string nombre_dataset(argv[1]);
    ifstream dataset(nombre_dataset);
    if (!dataset.is_open())
    {
        cout << "No se ha podido abrir el dataset" << endl << endl
             << nombre_dataset << endl << endl
             << "Comprueba que existe el archivo.";
        
        exit(2);
    }
    dataset.close();

    *atributo_a_eliminar = (int)strtol(argv[2], nullptr, 10) - 1;
    if (*atributo_a_eliminar < 0)
    {
        cout << "No se puede eliminar el atributo " << Miles(*atributo_a_eliminar + 1) << endl << endl;
        Uso(argv);
        exit(3);
    }

    string nombre_dataset_reducido(RutaArchivo(nombre_dataset) + NombreArchivo(nombre_dataset) + "-" + NumeroAString(*atributo_a_eliminar + 1) + "." + ExtensionArchivo(nombre_dataset));
    ofstream dataset_sin_atributo(nombre_dataset_reducido);
    if (!dataset_sin_atributo.is_open())
    {
        cout << "No se ha podido crear el dataset" << endl << endl
             << nombre_dataset_reducido << endl << endl
             << "Comprueba que los permisos de escritura en la carpeta" << endl << endl;
        cout << '\t' << RutaArchivo(nombre_dataset) << endl << endl;
        
        exit(4);
    }
    dataset_sin_atributo.close();

    if (argc == 4)
    {
        string argumento("--verbose");
        if (argumento.compare(argv[3]))
        {
            Uso(argv);
            exit(5);
        }
        
        *verbose = true;
    }
}


int main(int argc, const char * argv[])
{
    int atributo_a_eliminar;
    bool verbose = false;
    
    ProcesaArgumentos(argc, argv, &atributo_a_eliminar, &verbose);
    
    DECLARA_E_INICIA_RELOJ_TOTAL
    DECLARA_E_INICIA_RELOJ
    
    //Apertura del dataset
    string nombre_dataset(argv[1]);
    ifstream dataset(nombre_dataset);
    if (verbose)
        cout << "Analizando " << nombre_dataset << " (" << GetTamanyoArchivo(nombre_dataset) << ")" << endl << std::flush;
    
    ///< La clase puede ser el primer o último elemento de cada evidencia
    bool clase_al_final = true;
    
    TCatalogo D;
    
    vector<map<string, size_t>> Atributos_rango;
    
    int num_atributos;
    size_t num_clases_distintas,
           num_valores_distintos;
    
    //Lectura de evidencias completas
    if (verbose)
    {
        cout << "Leyendo " << NombreArchivo(nombre_dataset) << " " << std::flush;
        INICIA_RELOJ
    }
    size_t num_evidencias_dataset = LeeDataset(dataset, D, Atributos_rango, &num_atributos, clase_al_final/*´´, false*/);
    if (verbose)
    {
        MSG_TIEMPO_TRANSCURRIDO
        cout << endl << std::flush;
        
        GetValoresYClases(Atributos_rango, &num_clases_distintas, &num_valores_distintos);
        cout << "El dataset tiene " << Miles(num_evidencias_dataset) << " evidencias, " << Miles(num_valores_distintos) << " valores y " << Miles(num_clases_distintas) << " clases" << endl;
    }

    //Ya no se usará el dataset
    dataset.close();
    
    //TODO:Si obtuviera antes el número de argumentos, en ProcesaArgumentos...
    if (atributo_a_eliminar < 0 || atributo_a_eliminar >= num_atributos)
    {
        cout << "El experimento tiene " << Miles(num_atributos) << " atributos" << endl;
        cout << "No se puede eliminar el atributo " << Miles(atributo_a_eliminar + 1) << endl << endl;
        
        return 5;
    }
    else if (verbose)
        cout << "Se eliminará el atributo " << Miles(atributo_a_eliminar + 1) << endl << std::flush;
    
    ///< Eliminación de incertidumbre
    if (verbose)
    {
        cout << "Eliminando incertidumbre " << std::flush;
        INICIA_RELOJ
    }
    size_t num_caracterizaciones_completas_con_incertidumbre = EliminaIncertidumbre(D);
    if (verbose)
    {
        MSG_TIEMPO_TRANSCURRIDO
        cout << endl << std::flush;
    }
    
    if (verbose)
        cout << "Se han eliminando " << Miles(num_caracterizaciones_completas_con_incertidumbre) << " evidencias con incertidumbre " << endl << std::flush;

    //Conjuntos de atributos
    TSetEnteros Atributos_constantes,
                Atributos_redundantes,
                Atributos_esenciales,
                Atributos_eliminados;
    
    TCatalogo D_sin_atributo;
    
    ///< Creación del catálogo reducido
    bool creado = false;
    if (verbose)
    {
        cout << "Creando catálogo reducido " << std::flush;
        INICIA_RELOJ
    }
    creado = CreaCRsinAi(D, Atributos_eliminados, atributo_a_eliminar, num_atributos, D_sin_atributo);
    if (creado)
    {
        if (verbose)
        {
            MSG_TIEMPO_TRANSCURRIDO
            cout << endl << std::flush;
        }
        //Recalculo número de valores y clases tras eliminar incertidumbre
        GetValoresYClases(Atributos_rango, &num_clases_distintas, &num_valores_distintos);

        size_t num_valores_distintos_dataset_reducido = num_valores_distintos - Atributos_rango.at(atributo_a_eliminar).size();
        
        if (verbose)
        {
            cout << "El catálogo robusto original tiene " << Miles(D.size()) << " evidencias, " << Miles(num_valores_distintos) << " valores y " << Miles(num_clases_distintas) << " clases" << endl;
            cout << "El catálogo robusto reducido tiene " << Miles(D_sin_atributo.size()) << " evidencias y " << Miles(num_valores_distintos_dataset_reducido) << " valores" << endl;
        }
        
        if (verbose)
        {
            cout << "Guardando catálogo reducido " << std::flush;
            INICIA_RELOJ
        }
        string nombre_dataset_reducido(RutaArchivo(nombre_dataset) + NombreArchivo(nombre_dataset) + "-" + NumeroAString(atributo_a_eliminar + 1) + "." + ExtensionArchivo(nombre_dataset));
        ofstream dataset_sin_atributo(nombre_dataset_reducido);
        
        for (auto evidencia : D_sin_atributo) {
            dataset_sin_atributo << evidencia.first << "," << *evidencia.second.begin() << endl;
        }
        if (verbose)
        {
            MSG_TIEMPO_TRANSCURRIDO
            cout << endl << std::flush;
        }
    }
    else
    {
        if (verbose)
        {
            MSG_TIEMPO_TRANSCURRIDO
            cout << endl << std::flush;
        }
        cout << "Si se elimina el atributo " << Miles(atributo_a_eliminar + 1) << " no se obtiene un catálogo robusto" << endl << endl;
        
        return 0;
    }

    MSG_TIEMPO_TRANSCURRIDO_TOTAL
    cout << endl << endl;
    
    return 0;
}
