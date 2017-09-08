//  scd.h
//  lectura-scd
//
//  Created by Enrique Lazcorreta Puigmartí on 20/4/17.
//  Copyright $\comentarioCodigo{\copyright}$ 2017 Enrique Lazcorreta Puigmartí. Some rights reserved.
//
//  This file is part of ACDC.
//
//  ACDC is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ACDC is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with ACDC. If not, see <http://www.gnu.org/licenses/>.

#ifndef scd_h
#define scd_h


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

#include <algorithm>

#include <fstream>
using std::ifstream;
using std::ofstream;

typedef map<string, set<string>> TCatalogo;
typedef set<int> TSetEnteros;


inline void LeeLinea(ifstream &archivo, string &linea)
{
    std::getline(archivo, linea);
    
    if (!linea.empty())
    {
        if (linea.back() == '\r')
            linea.pop_back();
        
        size_t primer_caracter = linea.find_first_not_of(", ");
        
        if (primer_caracter)
            linea.assign(linea.substr(primer_caracter));
    }
}



size_t LeeDataset(ifstream &dataset,
                  TCatalogo &D_cero,
                  vector<map<string, size_t>> &Atributos,
                  int *num_atributos,
                  bool &clase_al_final,
                  bool lee_valores_y_clases = true)
{
    size_t num_evidencias_dataset = 0;
    
    //Comprobación de la posición de la clase
    string linea;
    LeeLinea(dataset, linea);
    
    if (!linea.compare("#clase_al_final"))
        LeeLinea(dataset, linea);
    else if (!linea.compare("#clase_al_principio"))
    {
        clase_al_final = false;
        LeeLinea(dataset, linea);
    }
    
    //Lectura de metadatos, si los tiene (se ignoran)
    char caracter_inicial = linea[0];
    while (caracter_inicial == '@' || caracter_inicial == '#' || linea.empty())
    {
        LeeLinea(dataset, linea);
        caracter_inicial = linea[0];
    }
    
    //Obtención del número de atributos a partir de la primera evidencia
    *num_atributos = -1;
    for (size_t posicion_1 = linea.find_first_not_of(" ,"); posicion_1 < string::npos; )
    {
        size_t posicion_2 = linea.find_first_of(" ,", posicion_1);
        posicion_1 = linea.find_first_not_of(" ,", posicion_2);
        (*num_atributos)++;
    }
    
    Atributos.resize(*num_atributos + 1);
    
    //Leemos todas las evidencias completas
    //TODO: Podrían leerse también las incompletas si lo pide el usuario. No implementado. Necesitaría otro parámetro, al menos para decir cuántas evidencias incompletas hay.
    while (dataset.good())
    {
        if (linea.empty() || linea.find('?') != string::npos)
        {
            LeeLinea(dataset, linea);
            continue;
        }
        
        string caracterizacion(linea),
               clase;
        
        if (clase_al_final)
        {
            clase = caracterizacion.substr(caracterizacion.find_last_of(", ") + 1);
            
            caracterizacion = caracterizacion.substr(0, caracterizacion.find_last_of(", "));
            caracterizacion = caracterizacion.substr(0, caracterizacion.find_last_not_of(", ") + 1);
        }
        else
        {
            clase = caracterizacion.substr(0, caracterizacion.find_first_of(", "));
            
            caracterizacion = caracterizacion.substr(caracterizacion.find_first_of(", ") + 1);
            caracterizacion = caracterizacion.substr(caracterizacion.find_first_not_of(", "));
        }
        
        D_cero[caracterizacion].insert(clase);
        
        if (lee_valores_y_clases)
        {
            size_t posicion_dato = linea.find_first_not_of(", ");
            
            for (int atributo = 0; atributo < *num_atributos; atributo++)
            {
                size_t posicion_separador = linea.find_first_of(" ,", posicion_dato);
                string dato(linea.substr(posicion_dato, posicion_separador - posicion_dato));
                Atributos.at(atributo)[dato]++;
                posicion_dato = linea.find_first_not_of(" ,", posicion_separador);
            }
            
            Atributos.at(*num_atributos)[clase]++;
        }
        
        num_evidencias_dataset++;
        
        LeeLinea(dataset, linea);
    }
    
    return num_evidencias_dataset;
}



size_t EliminaIncertidumbre(TCatalogo &D)
{
    size_t num_evidencias_con_y_sin_incertidumbre = D.size();
    
    for (TCatalogo::iterator evidencia = D.begin(); evidencia != D.end(); )
        if (evidencia->second.size() > 1)
            evidencia = D.erase(evidencia);
        else
            ++evidencia;
    
    return (num_evidencias_con_y_sin_incertidumbre - D.size());
}



void GetValoresYClases(vector<map<string, size_t>> &Atributos_rango,
                       size_t *num_clases_distintas,
                       size_t *num_valores_distintos)
{
    *num_clases_distintas = Atributos_rango.at(Atributos_rango.size() - 1).size();
    
    *num_valores_distintos = 0;
    for (auto atributo : Atributos_rango)
        *num_valores_distintos += atributo.size();
    *num_valores_distintos -= *num_clases_distintas;
}



inline const string ObtenSubCaracterizacion(const string &caracterizacion,
                                            const int atributo_a_eliminar,
                                            const int num_atributos)
{
    if (atributo_a_eliminar == 0)
        return caracterizacion.substr(caracterizacion.find_first_of(", ") + 1);
    
    if (atributo_a_eliminar == num_atributos - 1)
        return caracterizacion.substr(0 ,caracterizacion.find_last_of(", "));
    
    size_t posicion_inicial = 0,
           posicion_final = 0;
    
    for (int i = 0; i < atributo_a_eliminar; i++)
    {
        posicion_final = caracterizacion.find_first_of(", ", posicion_inicial);
        posicion_inicial = caracterizacion.find_first_not_of(", ", posicion_final);
    }
    
    return caracterizacion.substr(0, posicion_final) + "," + caracterizacion.substr(caracterizacion.find_first_of(", ", posicion_inicial) + 1);
}



inline int ColumnaAEliminar(const int i,
                            const int num_atributos,
                            const TSetEnteros &I)
{
    int columna_a_eliminar = 0;
    
    for (int atributo = 0 ; atributo < num_atributos; atributo++)
    {
        if (I.find(atributo) != I.end())
            continue;
        if (atributo == i)
            break;
        columna_a_eliminar++;
    }
    
    return  columna_a_eliminar;
}



bool CreaCatalogoRobustoSinAtributo_i(const TCatalogo &D_sin_I,
                                     const TSetEnteros &I,
                                     const int i,
                                     const int num_atributos,
                                     TCatalogo &D_sin_I_sin_i)
{
    int dato_a_eliminar = ColumnaAEliminar(i, num_atributos, I);
    
    for (TCatalogo::const_iterator evidencia = D_sin_I.begin(); evidencia != D_sin_I.end(); ++evidencia)
    {
        string caracterizacion(ObtenSubCaracterizacion(evidencia->first, dato_a_eliminar, num_atributos - (int)I.size()));
        D_sin_I_sin_i[caracterizacion].insert(*(evidencia->second.cbegin()));
        
        if (D_sin_I_sin_i[caracterizacion].size() > 1)
        {
            D_sin_I_sin_i.clear();
            return false;
        }
    }
    
    return true;
}



size_t CreaCCR(TCatalogo &D_sin_I,
               TSetEnteros &I,
               const int ultimo_atributo_eliminado,
               const int num_atributos,
               set<TSetEnteros> &CCR)
{
    static size_t num_llamadas = 0;
    num_llamadas++;
    
    for (int atributo_a_eliminar = ultimo_atributo_eliminado + 1;
         atributo_a_eliminar < num_atributos; atributo_a_eliminar++)
    {
        if (I.find(atributo_a_eliminar) != I.end())
            continue;
        
        I.insert(atributo_a_eliminar);
        
        //Comprobar si ya sabemos que I es robusto (si $\acdcDsinI$ es superconjunto de un catálogo robusto)
        
        
        
        
        
        for (TCatalogo::const_iterator evidencia = D_sin_I.begin(); evidencia != D_sin_I.end(); ++evidencia)
        {
            
        }
    }
    
    return num_llamadas;
}



void EliminaAtributoConstante(TCatalogo &D_sin_I,
                              TSetEnteros &I,
                              const int i,
                              const int num_atributos)
{
    TCatalogo D_reducido;
    
    for (auto atributo : D_sin_I)
    {
        string caracterizacion_sin_i(ObtenSubCaracterizacion(atributo.first, i,  num_atributos));
        D_reducido[caracterizacion_sin_i] = atributo.second;
    }
    
    D_sin_I.swap(D_reducido);
    I.insert(i);
}



void EliminaAtributosConstantes(TCatalogo &D_sin_I,
                                TSetEnteros &I,
                                const vector<map<string, size_t>> &Atributos_rango,
                                TSetEnteros &Atributos_constantes,
                                const int num_atributos)
{
    int indice_atributo = 0;

    for (vector<map<string, size_t>>::const_iterator atributo = Atributos_rango.begin(); atributo != Atributos_rango.end(); ++atributo)
    {
        if (atributo->size() == 1)
        {
            Atributos_constantes.insert(indice_atributo);
            EliminaAtributoConstante(D_sin_I, I, indice_atributo, num_atributos);
            I.insert(indice_atributo);
        }

        indice_atributo++;
    }
}

#endif /* scd_h */
