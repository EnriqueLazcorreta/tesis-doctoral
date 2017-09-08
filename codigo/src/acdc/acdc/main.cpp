//
//  main.cpp
//  lectura-scd
//
//  Created by Enrique Lazcorreta Puigmartí on 20/4/17.
//  Copyright $\copyright$ 2017 Enrique Lazcorreta Puigmartí. All rights reserved.
//

#include <iostream>
using std::cout;
using std::endl;
#include <iomanip>

#include <ctime>
#define DECLARA_E_INICIA_RELOJ_TOTAL clock_t start_TOTAL = clock();
#define TIEMPO_TRANSCURRIDO_TOTAL ((clock() - start_TOTAL) / double(CLOCKS_PER_SEC))

#define DECLARA_E_INICIA_RELOJ clock_t start = clock();
#define TIEMPO_TRANSCURRIDO ((clock() - start) / double(CLOCKS_PER_SEC))

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



string NombreAplicacion(const char *argv_0)
{
    string nombre_aplicacion(argv_0);
    nombre_aplicacion.assign(nombre_aplicacion.substr(nombre_aplicacion.find_last_of("/\\") + 1));
    return nombre_aplicacion;
}



string NombreArchivo(const string nombre_y_ruta)
{
    string nombre = nombre_y_ruta.substr(nombre_y_ruta.find_last_of("/\\") + 1);
    
    size_t posicion = nombre.find_last_of(".");
    
    return nombre.substr(0, posicion);
}



string TamanyoArchivoLegibleParaHumanos(fpos_t tamanyo_archivo)
{
    int unidad = 0;
    
    float tamanyo = (float)tamanyo_archivo;
    while (tamanyo > 1024 && unidad < 4)
    {
        tamanyo /= 1024;
        unidad++;
    }
    
    string resultado;
    char buffer[10];
    
    if (!unidad)
        sprintf(buffer, "%zu", (size_t)tamanyo);
    else
        sprintf(buffer, "%.2f", tamanyo);
    
    resultado.append(buffer);
    
    switch (unidad) {
        case 0: // Bytes
            resultado.append("B");
            break;
            
        case 1: // KB
            resultado.append("KB");
            break;
            
        case 2: // MB
            resultado.append("MB");
            break;
            
        case 3: // GB
            resultado.append("GB");
            break;
            
        case 4: // TB
            resultado.append("TB");
    }
    
    return resultado;
}



string GetTamanyoArchivo(const string &nombre_archivo)
{
    size_t tamanyo = 0;
    
    ifstream archivo(nombre_archivo);
    
    if (archivo.is_open())
    {
        size_t posicion_actual = archivo.tellg();
        archivo.seekg(0, archivo.end);
        tamanyo = (double)archivo.tellg();
        archivo.seekg(posicion_actual, archivo.beg);
    }
    
    archivo.close();
    
    return TamanyoArchivoLegibleParaHumanos(tamanyo);
}



#define SEPARADOR_DE_MILES ","
string Miles(size_t numero,
             const string separador_de_miles = SEPARADOR_DE_MILES)
{
    if (!numero)
        return "0";
    
    string numero_con_separaciones;
    char digitos_escritos = 0;
    
    while (numero)
    {
        numero_con_separaciones.insert(0, 1, (char)(numero - (numero / 10) * 10 + 48));
        
        numero /= 10;
        
        digitos_escritos++;
        if (digitos_escritos == 3 && numero)
        {
            numero_con_separaciones.insert(0, separador_de_miles);
            digitos_escritos = 0;
        }
    }
    
    return numero_con_separaciones;
}



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



string ObtenSubCaracterizacion(const string &caracterizacion,
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
        posicion_inicial = posicion_final + 1;
    }
    
    return caracterizacion.substr(0, posicion_final) + "," + caracterizacion.substr(caracterizacion.find_first_of(", ", posicion_inicial));
}



size_t EliminaIncertidumbre(map<string, set<string>> &D)
{
    size_t num_caracterizaciones = D.size();
    
    for (map<string, set<string>>::iterator evidencia = D.begin(); evidencia != D.end(); )
        if (evidencia->second.size() > 1)
            evidencia = D.erase(evidencia);
        else
            ++evidencia;
    
    return (num_caracterizaciones - D.size());
}



int main(int argc, const char * argv[])
{
    if (argc < 2)
    {
        cout << "Uso:" << endl << endl;
        cout << NombreAplicacion(argv[0]) << " listado-datasets [?]" << endl << endl;
        cout << "En el archivo listado-datasets está el listado de los "
        "datasets a analizar, cada uno en una línea y con la "
        "ruta completa del dataset." << endl
        << "El parámetro opcional ? indica que se lean también las "
        "evidencias incompletas como si no lo fueran." << endl;
        
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
    
    bool leer_evidencias_incompletas = false;
    if (argc > 2)
        leer_evidencias_incompletas = (argv[2][0] == '?');
    
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
    << "\tTamaño  = tamaño del archivo que contiene el dataset" << endl
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
    << "Tamaño  " << " & "
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
        
        //Comprobación de la posición de la clase
        string linea;
        LeeLinea(dataset, linea);
        
        bool clase_al_final = true;
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
        int num_atributos = -1;
        for (size_t posicion_1 = 0; posicion_1 < string::npos; )
        {
            size_t posicion_2 = linea.find_first_of(" ,", posicion_1);
            posicion_1 = linea.find_first_not_of(" ,", posicion_2);
            num_atributos++;
        }
        
        
        //Lectura de evidencias completas anotando (caracterización, {clases})
        map<string, set<string>> D_cero;
        
        //Se obtiene el rango de todos los atributos
        vector<map<string, size_t>> Atributo_rango(num_atributos + 1);
        
        size_t num_evidencias_dataset = 0;
        
        while (dataset.good())
        {
            if (!leer_evidencias_incompletas)
                if (linea.empty() || linea.find('?') != string::npos)
                {
                    LeeLinea(dataset, linea);
                    continue;
                }
            
            string caracterizacion = linea,
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
            
            size_t posicion_dato = 0;
            
            for (int j = 0; j < num_atributos + 1; j++)
            {
                size_t posicion_separador = linea.find_first_of(" ,", posicion_dato);
                string dato(linea.substr(posicion_dato, posicion_separador - posicion_dato));
                Atributo_rango.at(j)[dato]++;
                posicion_dato = linea.find_first_not_of(" ,", posicion_separador);
            }
            
            num_evidencias_dataset++;
            
            LeeLinea(dataset, linea);
        }
        
        
        //Ya no se usará el dataset
        dataset.close();
        
        
        //Número de clases
        size_t num_clases_distintas = clase_al_final ? Atributo_rango.at(num_atributos).size() : Atributo_rango.at(0).size();
        
        //Número de valores (A_i = x_i) distintos)
        size_t num_valores_distintos = 0;
        for (int j = 0; j < num_atributos + 1; j++)
            num_valores_distintos += Atributo_rango.at(j).size();
        num_valores_distintos -= num_clases_distintas;
        
        
//        //Eliminación de incertidumbre
//        map<string, string> D;
        
        //Número de evidencias completas distintas en el dataset y de su mayor catálogo robusto
        size_t num_evidencias_completas_dataset = D_cero.size();

        size_t num_caracterizaciones_completas_con_incertidumbre = EliminaIncertidumbre(D_cero);
//        size_t num_caracterizaciones_completas_con_incertidumbre = 0;
//        for (auto &evidencia : D_cero)
//            if (evidencia.second.size() > 1)
//                num_caracterizaciones_completas_con_incertidumbre++;
//            else
//                D[evidencia.first] = *evidencia.second.begin();
//        if (num_caracterizaciones_completas_con_incertidumbre)
//            num_datasets_con_incertidumbre++;
        
        size_t num_evidencias_completas_robustas = D_cero.size();
//        size_t num_evidencias_completas_robustas = D.size();
        
        if (num_evidencias_completas_robustas == num_evidencias_dataset)
            num_datasets_sin_duplicados++;
        
//        //Ya no necesitamos D_0
//        D_cero.clear();
        
        
        //Descubrir qué atributos son redundantes
        vector<bool> Atributo_redundante(num_atributos);
        
        size_t min_num_evidencias_robustas = num_evidencias_completas_robustas + 1,
        min_num_evidencias_robustas_valores = num_valores_distintos,
        min_num_valores = num_valores_distintos + 1,
        min_num_valores_evidencias = num_evidencias_completas_robustas;
        int min_num_evidencias_robustas_atributo = 0,
        min_num_valores_atributo = 0;
        
        for (int atributo = 0; atributo < num_atributos; atributo++)
        {
            map<string, set<string>> D_reducido;
            Atributo_redundante.at(atributo) = true;
            
            for (auto &evidencia : D_cero)
            {
                string caracterizacion(ObtenSubCaracterizacion(evidencia.first, atributo, num_atributos));
                if (D_reducido.find(caracterizacion) != D_reducido.end())
                {
                    Atributo_redundante.at(atributo) = false;
                    break;
                }
                else
                    D_reducido[caracterizacion].insert(*evidencia.second.begin());
//                string caracterizacion(ObtenSubCaracterizacion(e.first, atributo, num_atributos));
//                D_reducido[caracterizacion].insert(e.second);
//                
//                if (D_reducido[caracterizacion].size() > 1)
//                {
//                    Atributo_redundante.at(atributo) = false;
//                    break;
//                }
            }
            
            if (Atributo_redundante.at(atributo))
            {
                if (min_num_evidencias_robustas > D_reducido.size())
                {
                    min_num_evidencias_robustas = D_reducido.size();
                    min_num_evidencias_robustas_valores = num_valores_distintos - Atributo_rango.at(atributo).size();
                    min_num_evidencias_robustas_atributo = atributo + 1;
                }
                
                if (min_num_valores > num_valores_distintos - Atributo_rango.at(atributo).size())
                {
                    min_num_valores = num_valores_distintos - Atributo_rango.at(atributo).size();
                    min_num_valores_evidencias = D_reducido.size();
                    min_num_valores_atributo = atributo + 1;
                }
            }
        }
        
        int num_atributos_redundantes = 0;
        for (auto atributo : Atributo_redundante)
            if (atributo)
                num_atributos_redundantes++;
        if (num_atributos_redundantes)
            num_datasets_con_atributos_redundantes++;
        
        int num_atributos_constantes = 0;
        for (auto atributo : Atributo_rango)
            if (atributo.size() == 1)
                num_atributos_constantes++;
        if (num_atributos_constantes)
            num_datasets_con_atributos_constantes++;
        
        //Tabla formateada para LaTeX
        cout << Miles(num_datasets_analizados)<< " & "
        << NombreArchivo(nombre_dataset) << " & "
        << GetTamanyoArchivo(nombre_dataset) << " & "
        << Miles(num_atributos) << "("
        << Miles(num_atributos_redundantes) << ")("
        << (num_atributos_constantes ? Miles(num_atributos_constantes) : "-") << ") & "
        << Miles(num_valores_distintos) << " & "
        << Miles(num_clases_distintas) << " & "
        << Miles(num_evidencias_dataset) << " & "
        << Miles(num_evidencias_completas_dataset) << " & "
        << (num_caracterizaciones_completas_con_incertidumbre ? Miles(num_caracterizaciones_completas_con_incertidumbre) : " ") << " & ";
        
        if (num_atributos_redundantes)
            cout << "$A_{" << Miles(min_num_evidencias_robustas_atributo) << "}$ ("
            << Miles(min_num_evidencias_robustas_valores) << " / "
            << Miles(min_num_evidencias_robustas) << ") & "
            << "$A_{" <<  Miles(min_num_valores_atributo) << "}$ ("
            << Miles(min_num_valores) << " / "
            << Miles(min_num_valores_evidencias) << ") & ";
        else
            cout << "            &            & ";
        
        cout << std::setprecision(3) << std::fixed << TIEMPO_TRANSCURRIDO << " \\\\" << endl << "\\hline" << endl;
    }
    
    cout << "\\hline" << endl
    << "TOTAL &  &  & (" << Miles(num_datasets_con_atributos_redundantes) << ")(" << Miles(num_datasets_con_atributos_constantes) << ") &  &  & " << Miles(num_datasets_sin_duplicados) << " & " << Miles(num_datasets_con_incertidumbre) << " &  &  & " << TIEMPO_TRANSCURRIDO_TOTAL << endl;
    
    return 0;
}
