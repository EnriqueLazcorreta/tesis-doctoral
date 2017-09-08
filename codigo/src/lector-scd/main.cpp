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

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <fstream>
using std::ifstream;



int main(int argc, const char * argv[])
{
    if (argc < 2)
    {
        cout << "Uso:" << endl << endl;
        cout << NombreAplicacion(argv[0]) << " listado-datasets" << endl << endl;
        cout << "En el archivo listado-datasets está el listado de los "
        "datasets a analizar, cada uno en una línea y con la "
        "ruta completa del dataset." << endl;
        
        return 1;
    }
    
    ifstream listado_datasets(argv[1]);
    if (!listado_datasets.is_open())
    {
        cout << "No se ha podido abrir el listado" << endl << endl
        << argv[1] << endl << endl
        << "Comprueba que existe el archivo.";
        
        return 2;
    }
    else
        cout << argv[1] << " abierto" << endl;
    
    DECLARA_E_INICIA_RELOJ_TOTAL
    
    int num_datasets_analizados = 0,
        num_datasets_con_atributos_redundantes = 0,
        num_datasets_con_atributos_constantes = 0,
        num_datasets_sin_duplicados = 0,
        num_datasets_con_incertidumbre = 0;
    
    //Cabecera de la tabla
    cout << "Cabecera de la tabla" << endl
         << "\t#       = número del dataset analizado" << endl
         << "\tDataset = nombre del dataset" << endl
         << "\tTama$\tilde{n}$o  = tama$\tilde{n}$o del archivo que contiene el dataset" << endl
         << "\tN(r)(c) = número de atributos (redundantes) (constantes)" << endl
         << "\trg(A)   = número de valores distintos (suma de rangos de los atributos)" << endl
         << "\tQ       = número de clases" << endl
         << "\t|dat|   = número de evidencias del dataset" << endl
         << "\tM       = número de evidencias completas" << endl
         << "\tInc.    = número de caracterizaciones con incertidumbre" << endl
         << "\t(1)     = \"atributo eliminado (valores, evidencias)\" para minimizar evidencias" << endl
         << "\t(2)     = \"atributo eliminado (valores, evidencias)\" para minimizar valores" << endl
         << "\tt(sg)   = tiempo de ejecución, en segundos" << endl
         << endl;
    
    cout << "#   &   "
         << "Dataset " << " & "
         << "Tama$\tilde{n}$o  " << " & "
         << "N(r)(c) " << " & "
         << "rg(A)   " << " & "
         << "Q       " << " & "
         << "|dat|   " << " & "
         << "M       " << " & "
         << "Inc.    " << " & "
         << "(1)     " << " & "
         << "(2)     " << " & "
         << "t(sg)   " << " \\\\" << endl << "\\hline"
         << endl;
    
    while (listado_datasets.good())
    {
        DECLARA_E_INICIA_RELOJ
        
        //Apertura del dataset
        string nombre_dataset;
        LeeLinea(listado_datasets, nombre_dataset);
        ifstream dataset(nombre_dataset);
        if (nombre_dataset.empty())
            continue;
        
        if (!dataset.is_open())
        {
            cout << "******* " << nombre_dataset << " no abierto *******" << endl;
            continue;
        }
        
        num_datasets_analizados++;
        
        ///< La clase puede ser el primer o último elemento de cada evidencia
        bool clase_al_final = true;
        
        TCatalogo D;
        
        vector<map<string, size_t>> Atributos_rango;
        
        int num_atributos;

        //Lectura de evidencias completas
        size_t num_evidencias_dataset = LeeDataset(dataset,
                                                   D,
                                                   Atributos_rango,
                                                   &num_atributos,
                                                   clase_al_final);
        
        //Ya no se usará el dataset
        dataset.close();
        
        int indice_primer_atributo = clase_al_final ? 0 : 1,
            indice_ultimo_atributo = clase_al_final ? num_atributos - 1 : num_atributos - 2,
            indice_clase = clase_al_final ? num_atributos : 0;

        size_t num_clases_distintas = Atributos_rango.at(indice_clase).size();
        
        size_t num_valores_distintos = 0;
        for (auto atributo : Atributos_rango)
            num_valores_distintos += atributo.size();
        num_valores_distintos -= num_clases_distintas;
        
        size_t num_evidencias_completas_dataset = D.size();
        
        ///< Eliminación de incertidumbre
        size_t num_caracterizaciones_completas_con_incertidumbre = EliminaIncertidumbre(D);

        if (num_caracterizaciones_completas_con_incertidumbre)
            num_datasets_con_incertidumbre++;
        
        size_t num_evidencias_completas_robustas = D.size();
        
        if (num_evidencias_completas_robustas == num_evidencias_dataset)
            num_datasets_sin_duplicados++;
        
        //Conjuntos de atributos
        TSetEnteros Atributos_constantes,
                    Atributos_redundantes,
                    Atributos_esenciales;
        
        //Generar la Colección de Catálogos Robustos
        size_t num_evidencias_al_minimizar_evidencias = num_evidencias_completas_robustas + 1,
               num_valores_al_minimizar_evidencias = num_valores_distintos,
               num_evidencias_al_minimizar_valores = num_evidencias_completas_robustas,
               num_valores_al_minimizar_valores = num_valores_distintos + 1;
        int atributo_que_minimiza_num_evidencias = 0,
            atributo_que_minimiza_num_valores = 0;

        for (int atributo = indice_primer_atributo; atributo <= indice_ultimo_atributo; atributo++)
        {
            if (Atributos_rango.at(atributo).size() == 1)
            {
                Atributos_constantes.insert(atributo);
                continue;
            }
            
            TCatalogo D_reducido;
            
            for (TCatalogo::const_iterator evidencia = D.begin();
                 evidencia != D.end(); ++evidencia)
            {
                string caracterizacion(ObtenSubCaracterizacion(evidencia->first,
                                                               atributo,
                                                               num_atributos));
                D_reducido[caracterizacion].insert(*(evidencia->second.cbegin()));
                if (D_reducido[caracterizacion].size() > 1)
                {
                    Atributos_esenciales.insert(atributo);
                    break;
                }
            }

            //TODO:Comprobar si find devuelve end() o npos (como string)
            if (Atributos_esenciales.find(atributo) == Atributos_esenciales.end())
            {
                if (num_evidencias_al_minimizar_evidencias > D_reducido.size())
                {
                    num_evidencias_al_minimizar_evidencias = D_reducido.size();
                    num_valores_al_minimizar_evidencias = num_valores_distintos - Atributos_rango.at(atributo).size();
                    atributo_que_minimiza_num_evidencias = atributo + 1;
                }
                
                if (num_valores_al_minimizar_valores > num_valores_distintos - Atributos_rango.at(atributo).size())
                {
                    num_valores_al_minimizar_valores = num_valores_distintos - Atributos_rango.at(atributo).size();
                    num_evidencias_al_minimizar_valores = D_reducido.size();
                    atributo_que_minimiza_num_valores = atributo + 1;
                }
            }
        }
        
        if (Atributos_esenciales.size() + Atributos_constantes.size() < num_atributos)
            num_datasets_con_atributos_redundantes++;
        
        if (Atributos_constantes.size())
        {
            num_datasets_con_atributos_constantes++;
//            num_datasets_con_atributos_redundantes++;
        }
        
        //Tabla formateada para LaTeX
        cout << Miles(num_datasets_analizados)<< " & "
             << NombreArchivo(nombre_dataset) << " & "
             << GetTamanyoArchivo(nombre_dataset) << " & ";
        
        //\newcommand{\todos}[2][-]{{\color{teal}#2{\tiny (#2)}{\tiny (#1)}}}
        //\newcommand{\ninguno}[2][-]{{\color{red}#2{\tiny (0)}{\tiny (#1)}}}
        //\newcommand{\alguno}[3][-]{#2{\tiny (#3)}{\tiny (#1)}}
        string msg_parametro_constantes = Atributos_constantes.size() ?
                                "[" + Miles(Atributos_constantes.size()) + "]" : "";
        int num_atributos_redundantes = num_atributos - (int)Atributos_esenciales.size() - (int)Atributos_constantes.size();
        string msg_atributos;
        if (num_atributos == num_atributos_redundantes + Atributos_constantes.size())
            msg_atributos = "\\todos" + msg_parametro_constantes + "{" + Miles(num_atributos) + "}";
        else if (!num_atributos_redundantes)
            msg_atributos = "\\ninguno" + msg_parametro_constantes + "{" + Miles(num_atributos) + "}";
        else
            msg_atributos = "\\alguno" + msg_parametro_constantes + "{" + Miles(num_atributos) + "}{" + Miles(num_atributos_redundantes) + "}";
        cout << msg_atributos << " & "
             << Miles(num_valores_distintos) << " & "
             << Miles(num_clases_distintas) << " & "
             << Miles(num_evidencias_dataset) << " & ";
        
        //\newcommand{\sinDuplicados}[1]{{\color{teal}#1}}
        string msg_num_evidencias_completas;
        if (num_evidencias_completas_dataset == num_evidencias_dataset)
            msg_num_evidencias_completas = "\\sinDuplicados{" + Miles(num_evidencias_completas_dataset) + "}";
        else
            msg_num_evidencias_completas = Miles(num_evidencias_completas_dataset);
        cout << msg_num_evidencias_completas << " & "
             << (num_caracterizaciones_completas_con_incertidumbre ? Miles(num_caracterizaciones_completas_con_incertidumbre) : "-") << " & ";
        
        if (num_atributos_redundantes)
            cout << "$A_{" << Miles(atributo_que_minimiza_num_evidencias) << "}$ ("
                 << Miles(num_valores_al_minimizar_evidencias) << " / "
                 << Miles(num_evidencias_al_minimizar_evidencias) << ") & "
                 << "$A_{" <<  Miles(atributo_que_minimiza_num_valores) << "}$ ("
                 << Miles(num_valores_al_minimizar_valores) << " / "
                 << Miles(num_evidencias_al_minimizar_valores) << ") & ";
        else
            cout << "            &            & ";
        
        cout << std::setprecision(4) << std::fixed << TIEMPO_TRANSCURRIDO << " \\\\" << endl << "\\hline" << endl;
    }
    
    cout << endl
         << "TOTAL &  &  & ("
         << Miles(num_datasets_con_atributos_redundantes) << ")("
         << Miles(num_datasets_con_atributos_constantes) << ") &  &  & "
         << Miles(num_datasets_sin_duplicados) << " & "
         << Miles(num_datasets_con_incertidumbre) << " &  &  & "
         << std::setprecision(4) << std::fixed << TIEMPO_TRANSCURRIDO_TOTAL << endl;
    
    return 0;
}
