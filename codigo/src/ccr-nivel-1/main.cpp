//  main.cpp
//  ccr-nivel-1
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

#include <set>
using std::set;

#include <fstream>
using std::ifstream;
using std::ofstream;

typedef map<string, set<string>> TCatalogo;
typedef set<int> TSetEnteros;



void Uso(const char * argv[])
{
    cout << "Uso:" << endl << endl;
    cout << '\t' << NombreAplicacion(argv[0]) << " dataset-de-clasificacion" << endl << endl;
    cout << "El archivo dataset-de-clasificacion es de texto plano separado "
    "por comas (formato CSV)." << endl << endl;
    cout << "Puede contener metadatos en las primeras líneas si su primer "
    "caracter es '@' o '#'." << endl << endl;
    cout << "La clase es el último dato de cada línea. Sólo si la primera "
    "línea del dataset contiene \"#clase_al_principio\" se "
    "tomará el primer dato como la clase del experimento." << endl << endl;
}



void ProcesaArgumentos(const int argc, const char * argv[])
{
    if (argc != 2)
    {
        Uso(argv);
        exit(1);
    }
}


int main(int argc, const char * argv[])
{
    ProcesaArgumentos(argc, argv);
    
    DECLARA_E_INICIA_RELOJ_TOTAL
    
    //Apertura del dataset
    string nombre_dataset(argv[1]);
    ifstream dataset(nombre_dataset);
    if (!dataset.is_open())
    {
        cout << "No se ha podido abrir el dataset" << endl << endl
        << nombre_dataset << endl << endl
        << "Comprueba que existe el archivo.";
        
        return 2;
    }
    else
        cout << "Analizando " << nombre_dataset << endl;
    
    ///< La clase puede ser el primer o último elemento de cada evidencia
    bool clase_al_final = true;
    
    TCatalogo D;
    
    vector<map<string, size_t>> Atributos_rango;
    
    int num_atributos;
    
    //Lectura de evidencias completas
    size_t num_evidencias_dataset = LeeDataset(dataset, D, Atributos_rango, &num_atributos, clase_al_final);
    
    //Ya no se usará el dataset
    dataset.close();
    
    size_t num_clases_distintas,
    num_valores_distintos;
    
    GetValoresYClases(Atributos_rango, &num_clases_distintas, &num_valores_distintos);
    
    size_t num_evidencias_completas_dataset = D.size();
    
    //Conjuntos de atributos
    TSetEnteros Atributos_constantes,
    Atributos_redundantes,
    Atributos_esenciales,
    Atributos_eliminados;
    
    //Los atributos constantes se han de eliminar desde el principio
    EliminaAtributosConstantes(D, Atributos_eliminados, Atributos_rango, Atributos_constantes, num_atributos);
    
    if (Atributos_constantes.size())
    {
        cout << "Se han eliminado " << Atributos_constantes.size() << " atributos constantes:";
        for (auto atributo : Atributos_constantes)
            cout << " A" << atributo + 1;
        cout << endl;
        
    }
    
    ///< Eliminación de incertidumbre
    size_t num_caracterizaciones_completas_con_incertidumbre = EliminaIncertidumbre(D);
    
    size_t num_evidencias_completas_robustas = D.size();
    
    //TODO:Estas variables sólo son necesarias si se va ha ejecutar ACDC de forma supervisada
    size_t num_evidencias_al_minimizar_evidencias = num_evidencias_completas_robustas + 1,
    num_valores_al_minimizar_evidencias = num_valores_distintos,
    num_evidencias_al_minimizar_valores = num_evidencias_completas_robustas,
    num_valores_al_minimizar_valores = num_valores_distintos + 1;
    int atributo_que_minimiza_num_evidencias = 0,
    atributo_que_minimiza_num_valores = 0;
    
    for (int atributo = 0; atributo < num_atributos; atributo++)
    {
        if (Atributos_eliminados.find(atributo) != Atributos_eliminados.end())
            continue;
        
        TCatalogo D_sin_atributo;
        
        if (CreaCRsinAi(D, Atributos_eliminados, atributo, num_atributos, D_sin_atributo))
        {
            cout << "A" << atributo + 1 << ": \t" << Miles(Atributos_rango.at(atributo).size()) << " \t" << Miles(D_sin_atributo.size()) << endl;
            
            Atributos_redundantes.insert(atributo);
            
            if (num_evidencias_al_minimizar_evidencias > D_sin_atributo.size())
            {
                num_evidencias_al_minimizar_evidencias = D_sin_atributo.size();
                num_valores_al_minimizar_evidencias = num_valores_distintos - Atributos_rango.at(atributo).size();
                atributo_que_minimiza_num_evidencias = atributo;
            }
            
            if (num_valores_al_minimizar_valores > num_valores_distintos - Atributos_rango.at(atributo).size())
            {
                num_valores_al_minimizar_valores = num_valores_distintos - Atributos_rango.at(atributo).size();
                num_evidencias_al_minimizar_valores = D_sin_atributo.size();
                atributo_que_minimiza_num_valores = atributo;
            }
        }
        else
            Atributos_esenciales.insert(atributo);
    }
    
    
    //Tabla formateada para LaTeX
    cout << NombreArchivo(nombre_dataset) << " & "
    << GetTamanyoArchivo(nombre_dataset) << " & ";
    
    //\newcommand{\todos}[2][-]{{\color{teal}#2{\tiny (#2)}{\tiny (#1)}}}
    //\newcommand{\ninguno}[2][-]{{\color{red}#2{\tiny (0)}{\tiny (#1)}}}
    //\newcommand{\alguno}[3][-]{#2{\tiny (#3)}{\tiny (#1)}}
    string msg_parametro_constantes = Atributos_constantes.size() ? "[" + Miles(Atributos_constantes.size()) + "]" : "";
    
    string msg_atributos;
    if (num_atributos == Atributos_redundantes.size() + Atributos_constantes.size())
        msg_atributos = "\\todos" + msg_parametro_constantes + "{" + Miles(num_atributos) + "}";
    else if (!(Atributos_redundantes.size() + Atributos_constantes.size()))
        msg_atributos = "\\ninguno" + msg_parametro_constantes + "{" + Miles(num_atributos) + "}";
    else
        msg_atributos = "\\alguno" + msg_parametro_constantes + "{" + Miles(num_atributos) + "}{" + Miles(Atributos_redundantes.size() + Atributos_constantes.size()) + "}";
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
    
    if (Atributos_redundantes.size())
        cout << "$A_{" << Miles(atributo_que_minimiza_num_evidencias + 1) << "}$ ("
        << Miles(num_valores_al_minimizar_evidencias) << " / "
        << Miles(num_evidencias_al_minimizar_evidencias) << ") & "
        << "$A_{" <<  Miles(atributo_que_minimiza_num_valores + 1) << "}$ ("
        << Miles(num_valores_al_minimizar_valores) << " / "
        << Miles(num_evidencias_al_minimizar_valores) << ") & ";
    else
        cout << "            &            & ";
    
    cout << std::setprecision(4) << std::fixed << TIEMPO_TRANSCURRIDO_TOTAL << " \\\\" << endl << "\\hline" << endl;
    
    //Generar la Colección de Catálogos Robustos
    
    
    return 0;
}
