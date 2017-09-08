//  comun.h
//  lectura-scd
//
//  Created by Enrique Lazcorreta Puigmartí on 20/4/17.
//  Copyright $\copyright$ 2017 Enrique Lazcorreta Puigmartí. All rights reserved.
//

#ifndef comun_h
#define comun_h

#include <ctime>
#define DECLARA_E_INICIA_RELOJ_TOTAL clock_t start_TOTAL = clock();
#define TIEMPO_TRANSCURRIDO_TOTAL ((clock() - start_TOTAL) / double(CLOCKS_PER_SEC))
#define MSG_TIEMPO_TRANSCURRIDO_TOTAL cout << std::setprecision(4) << std::fixed << "[" << TIEMPO_TRANSCURRIDO_TOTAL << "sg]";

#define DECLARA_E_INICIA_RELOJ clock_t start = clock();
#define INICIA_RELOJ start = clock();
#define TIEMPO_TRANSCURRIDO ((clock() - start) / double(CLOCKS_PER_SEC))
#define MSG_TIEMPO_TRANSCURRIDO cout << std::setprecision(4) << std::fixed << "(" << TIEMPO_TRANSCURRIDO << "sg)";

#include <string>
using std::string;

#include <fstream>
using std::ifstream;

#include <sstream>
using std::stringstream;

#include <iomanip>



string NombreAplicacion(const char *argv_0)
{
    string nombre_aplicacion(argv_0);
    nombre_aplicacion = nombre_aplicacion.substr(nombre_aplicacion.find_last_of("/\\") + 1);
    return nombre_aplicacion;
}



string NombreArchivo(const string &nombre_y_ruta)
{
    string nombre = nombre_y_ruta.substr(nombre_y_ruta.find_last_of("/\\") + 1);
    
    size_t posicion = nombre.find_last_of(".");
    
    return nombre.substr(0, posicion);
}






string ExtensionArchivo(const string &nombre_y_ruta)
{
    string extension = nombre_y_ruta.substr(nombre_y_ruta.find_last_of("/\\") + 1);
    
    size_t posicion = extension.find_last_of(".");
    
    if (posicion != string::npos)
        return extension.substr(posicion + 1);
    
    return "";
}






string RutaArchivo(const string nombre_y_ruta)
{
    string ruta(nombre_y_ruta.substr(0, nombre_y_ruta.find_last_of("/\\") + 1));
    return ruta;
}



string TamanyoArchivoLegibleParaHumanos(size_t tamanyo_archivo)
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



template <typename T>
string NumeroAString(T numero,
                     const int precision = 2)
{
    stringstream ss;
    ss << std::setprecision(precision) << std::fixed << numero;
    return ss.str();
}

#endif /* comun_h */
