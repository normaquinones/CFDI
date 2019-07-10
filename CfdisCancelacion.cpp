//---------------------------------------------------------------------------

#pragma hdrstop

#include "CfdisCancelacion.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
 /*!
* \brief      Función que nos permite actualizar los datos de la tabla CFDI_CANCELACION en cualquier situación de exito o error  y a su vez incocar a la funcion  que permite descontar o aumentar  del folio  según la sesión
* \author     Programador win A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]  DatosCfdi: Datos necesario del CFDI a cancelar
* @param[in]  NuevoEstado: estado en el que se encuentra el CFDI entrante
* @param[in]  OpCfdi:  operación que se realisará con el CFDI en la tabla CFDI_CANCELACION  puede ser actualizar, eliminar   o no hacer nada
* @param[in]  OpPaquete:  operación que se realizara con paquete si la sesión es electronica la cual puede ser devolver el folio,descontar el folio o no hacer nada
* @param[in]  &objDatosConexion: objeto que  contiene los parámetro necesarios de la base de datos a conectarse
* @param[out]
*/
void Cfdis::Actualiza(TDatosDocumento DatosCfdi, TEstadoCfdi NuevoEstado, TOperacionCfdi OpCfdi, TOperacionPaquete OpPaquete,TDatosDocumento &objDatosConexion)
{
   _InformationalLog(strLog+"Inicio de la funcion Actualiza", G8_TOKEN_CANCELACION);
	if ( OpCfdi != OpCfdiNinguna && DatosCfdi.Id < 0 && DatosCfdi.Uuid.IsEmpty() )
    {
        _ErrorLog(strLog+"OPCfdi= 0(Nada) && idDocumento < 0 && NO contiene UUID", G8_TOKEN_CANCELACION);
		throw ExceptionServidor(ErrorGenericoTimbrado, MODULO_CFDIS);
	}

	TList *Parametros = NULL;
	int Intentos = 1;
	UnicodeString Cfdi;
	Cfdi = DatosCfdi.CfdiComoCadena;

	if(DatosCfdi.IdProceso==ProcesoTimbrar )
	{
	  if ( NuevoEstado == EstCfdiTimbrado && (!Cfdi.Pos("<tfd:TimbreFiscalDigital")) )
	  {
		UnicodeString strTextoLogErrorUUID = (Cfdi.SubString( Cfdi.Pos( "folio" ) ,39))+(Cfdi.SubString( Cfdi.Pos( "Emisor rfc" ) ,28))+( Cfdi.SubString( Cfdi.Pos( "Receptor rfc" ) ,28)) ;
        _WarningLog(strLog+"El nuevo estado es 4(Timbrado) && El documento no contiene el nodo timbre", G8_TOKEN_CANCELACION);
		NuevoEstado =  EstCfdiPendiente;
        _WarningLog(strLog+"Se cambia a estado pendiente y se ejecuta la funcion Actualiza", G8_TOKEN_CANCELACION);
		Actualiza(DatosCfdi, EstCfdiPendiente, OpCfdiActualizar, OpPaqDecrementarYLiberar,objDatosConexion);
		DatosCfdi.ErrorSinUUID = true;
	  }
	}


	if(!DatosCfdi.ErrorSinUUID)
	{
	  while ( true )
	  {
		try
		{
			Parametros = new TList();
			_DebugLog(strLog+"Antes de ejecutar P. Almacenado ActualizaCancelaciones ", G8_TOKEN_CANCELACION);
			Bd::AgregaParametro(Parametros, "@Cfdi", DatosCfdi.Cfdi->XML->Text, TDBXDataTypes::WideStringType);
			Bd::AgregaParametro(Parametros, "@CfdiComoCadena", DatosCfdi.CfdiComoCadena, TDBXDataTypes::WideStringType);
			Bd::AgregaParametro(Parametros, "@Uuid", DatosCfdi.Uuid, TDBXDataTypes::WideStringType);
			Bd::AgregaParametro(Parametros, "@TipoDoc",DatosCfdi.TipoDoc, TDBXDataTypes::Int32Type);
			Bd::AgregaParametro(Parametros, "@DatosCfdi", DatosCfdi.DatosComoXml(), TDBXDataTypes::WideStringType);
			Bd::AgregaParametro(Parametros, "@NuevoEstado", NuevoEstado, TDBXDataTypes::Int32Type);
			Bd::AgregaParametro(Parametros, "@OpCfdi", OpCfdi, TDBXDataTypes::Int32Type);
			Bd::AgregaParametro(Parametros, "@OpPaquete", OpPaquete, TDBXDataTypes::Int32Type);
			Bd::AgregaParametro(Parametros, "@Acuse", DatosCfdi.Acuse, TDBXDataTypes::WideStringType);
			Bd::EjecutaProcedimiento("ActualizaCancelaciones", Parametros,objDatosConexion.Alias,objDatosConexion.Host,objDatosConexion.UsuarioBD,objDatosConexion.Password,objDatosConexion.nombreBD,objDatosConexion.MaxConnections);
            _DebugLog(strLog+"Despues de ejecutar P. Almacenado ActualizaCancelaciones ", G8_TOKEN_CANCELACION);
			   if(DatosCfdi.TipoSesion==1 && DatosCfdi.esFacturaMovil==2 || DatosCfdi.TipoSesion==2 && DatosCfdi.esFacturaMovil==2 ){
				 DatosCfdi.TipoSesion=-1;
			  }else{
					if(DatosCfdi.TipoSesion==1){
						DatosCfdi.TipoSesion=0;
					  }
			  }
			ProcesarFolios(DatosCfdi,NuevoEstado,OpCfdi,OpPaquete);
			Bd::BorraParametros(Parametros);
			delete Parametros;
			Parametros = NULL;

			_InformationalLog(strLog+"Fin de la funcion Actualiza", G8_TOKEN_CANCELACION);
			break;
		}
		catch(Exception &e)
		{
            _ErrorLog(strLog+"Inicio del catch de la funcion de Actualiza", G8_TOKEN_CANCELACION);
			if ( Parametros ){ /*<!JRMC 10/12/2018 se modifica por RQ-CFDI-844 para liberar correctamente el objeto Tlist*/
            	Bd::BorraParametros(Parametros);
                delete Parametros;
                Parametros = NULL;
                }

			if ( Intentos == NumIntentosActualizarCfdi )
			{
                UnicodeString Mensaje = StringReplace(ContenidoCorreoErrorActualizarCfdi, L"%CodigoError%", e.HelpContext, TReplaceFlags() << rfReplaceAll);
				Mensaje = StringReplace(Mensaje, L"%MensajeError%", e.Message, TReplaceFlags() << rfReplaceAll);
				Mensaje = StringReplace(Mensaje, L"%Id%", DatosCfdi.Id, TReplaceFlags() << rfReplaceAll);
				Mensaje = StringReplace(Mensaje, L"%Cfdi%", DatosCfdi.Cfdi->XML->Text, TReplaceFlags() << rfReplaceAll);
                _ErrorLog(strLog+"Fin del catch de la funcion de Actualiza", G8_TOKEN_CANCELACION);
				throw Exception(e.Message,e.HelpContext);
			}
			Intentos++;
		}
	  }
	}
}
//------------------------------------------------------------------------------
/*!
* \brief      Disminuir los folios disponibles en el paquete  del cliente   o Aumentar los folios cosumidos en su número de Serie
* \author     Programador win A Josué Martín Canto
* \version    5.0.0.0
* \date       20/03/2018
* @param[in]  TDatosDocumento DatosCfdi,TEstadoCfdi NuevoEstado,TOperacionCfdi OpCfdi,TOperacionPaquete OpPaquete
* @param[out]  sin parámetros de salida.
*/
void Cfdis::ProcesarFolios(TDatosDocumento DatosCfdi,TEstadoCfdi NuevoEstado,TOperacionCfdi OpCfdi,TOperacionPaquete OpPaquete)
{
   _InformationalLog(strLog+"Inicio de la funcion ProcesarFolios", G8_TOKEN_CANCELACION);
   TList *Parametros = NULL;
   /*<!Inicia JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
    try{
        try{
              Parametros = new TList();
              _DebugLog(strLog+"Antes de ejecutar P. Almacenado Cfdi33_ProcesaFolioDocCfdi", G8_TOKEN_CANCELACION);
              Bd::AgregaParametro(Parametros, "@idEmisor",DatosCfdi.IdEmisor, TDBXDataTypes::Int32Type);
              Bd::AgregaParametro(Parametros, "@idCfdi", DatosCfdi.TipoDoc, TDBXDataTypes::Int32Type);
              Bd::AgregaParametro(Parametros, "@TipoSesion", DatosCfdi.TipoSesion, TDBXDataTypes::Int32Type);
              Bd::AgregaParametro(Parametros, "@OpCfdi", OpCfdi, TDBXDataTypes::Int32Type);
              Bd::AgregaParametro(Parametros, "@OpPaquete", OpPaquete, TDBXDataTypes::Int32Type);
              Bd::AgregaParametro(Parametros, "@IdNumSerie", DatosCfdi.IdSerie, TDBXDataTypes::Int32Type);
              Bd::EjecutaProcedimiento("Cfdi33_ProcesaFolioDocCfdi", Parametros,"","","","","","");
              _DebugLog(strLog+"Despues de ejecutar P. Almacenado Cfdi33_ProcesaFolioDocCfdi", G8_TOKEN_CANCELACION);

          }
          catch(Exception &e)
          {
                _ErrorLog(strLog+"Inicio del catch de la funcion ProcesarFolios: CodigoError= "+IntToStr(e.HelpContext), G8_TOKEN_CANCELACION);
               throw Exception(e.Message,e.HelpContext);
          }
       }__finally{
           if(Parametros){
             Bd::BorraParametros(Parametros);
          	 delete Parametros;
         	 Parametros = NULL;
           }

       }
    /*<!Termina JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
   _InformationalLog(strLog+"Fin de la funcion ProcesarFolios", G8_TOKEN_CANCELACION);
}

//------------------------------------------------------------------------------
/*!
* \brief      Función que permite elegir el pac, modificar el pac  default, cancelar  y actualizar los datos del pac   con el cual se canceló el CFDI
* \author     Programador win A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]  &DatosCfdi: objeto de la Clase TDatosDocumento que contiene la información del CFDI a cancelar
* @param[in]  &Resultado
* @param[in]  &MensajeResultado: cadena que guarda el mensaje de error en caso de suceder algo durante el proceso
* @param[in]  &Acuse: cadena que guarda el acuse del SAT en caso de cancelarse con éxito
* @param[in]  &objDatosConexion: objeto que contiene los datos de la base de datos a conectarse
* @param[out]
*/
void Cfdis::Cancela(TDatosDocumento &DatosCfdi, _di_IXMLDocument &Resultado,UnicodeString &MensajeResultado, int &CodigoResultado, UnicodeString &Acuse,TDatosDocumento &objDatosConexion)
{
    _InformationalLog(strLog+"Inicio de la funcion Cancela", G8_TOKEN_CANCELACION);
	_IniciaTicks;
	bool InfoPACActualizada = false;

	try
	{
    	EligePacParaCancelar(DatosCfdi);

		if ( DatosCfdi.IdPAC == -1 || DatosCfdi.IdPAC == 0  )
		{
			DatosCfdi.IdPAC = StrToIntDef(Utileria::LeeIniConfig(NombreSeccionGeneral, NombreSeccionPacCancelDefecto, L"7"),7);
            _DebugLog(strLog+"El idPac = -1 || 0, Se obtiene la etiqueta  PacDefaultCancel="+DatosCfdi.IdPAC+"  de la sesion [General]", G8_TOKEN_CANCELACION);
		}

		ActualizaPacDeCfdiParaCancelar(DatosCfdi,objDatosConexion);
		EjecutaCancelacion(DatosCfdi, Resultado, MensajeResultado, CodigoResultado, Acuse);
        ActualizaInfoSeleccionPACParaCancelar(DatosCfdi);

		InfoPACActualizada = true;
    _InformationalLog(strLog+"Fin de la funcion Cancela", G8_TOKEN_CANCELACION);
	}
	catch(Exception &e)
	{
		_ErrorLog(strLog+"Inicio del catch de la funcion Cancela", G8_TOKEN_CANCELACION);
		if ( !InfoPACActualizada )
		{
				ActualizaInfoSeleccionPACParaCancelar(DatosCfdi);
		}
        _ErrorLog(strLog+"Fin del catch de la funcion Cancela", G8_TOKEN_CANCELACION);
		throw Exception(e.Message,e.HelpContext);
	}

}
//---------------------------------------------------------------------------
/*!
* \brief      Validar que los datos del cliente esten vigentes.
* \author     Programador win A Josué Martín Canto
* \version    5.0.0.0
* \date       20/03/2018
* @param[in]  UnicodeString Cfdi, UnicodeString Info, TDatosDocumento &DatosCfdi
* @param[out]
*/
void Cfdis::ValidacionesPreviasCancelaciones(UnicodeString Cfdi, UnicodeString Info, TDatosDocumento &DatosCfdi)
{
	_InformationalLog(strLog+"Inicio de la funcion ValidacionesPreviasCancelaciones", G8_TOKEN_CANCELACION);
	TClientDataSet *CdsResultado = NULL;
	TList *Parametros = NULL;
	int Resultado;
    UnicodeString strTipo="";
    /*<!Inicia JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
    try{
            try{

                _di_IXMLDocument InfoXml = Utileria::dameNuevoDocXML(Info);
                _DebugLog(strLog+"Se convirtio a XML el  XMl de funciones50", G8_TOKEN_CANCELACION);
                DatosCfdi.Cfdi = Utileria::dameNuevoDocXML(Cfdi);

                try
                {
                    DatosCfdi.FechaCancelacion = Raiz(DatosCfdi.Cfdi)->Attributes["Fecha"];
                    Utileria::FechaCfdiAFecha(DatosCfdi.FechaCancelacion);
                    _DebugLog(strLog+"Se valido correctamente la estructura d ela fecha del documento: Fecha="+DatosCfdi.FechaCancelacion, G8_TOKEN_CANCELACION);
                }
                catch(Exception &e)	{ throw ExceptionServidor(ErrorFechaInvalidaCancelacion, MODULO_CFDIS); }
                int ServidorQueProcesa = StrToIntDef(Utileria::LeeIniConfig(NombreSeccionGeneral,IdServidorProcesaPeticion,IntToStr(ValorDefectoIdServidorProcesaPeticion)),ValorDefectoIdServidorProcesaPeticion);

                Raiz(InfoXml)->AddChild("IdServidor")->Text =  " ["+UnicodeString(ServidorQueProcesa)+"]";
                Raiz(InfoXml)->AddChild("Ip")->Text = UnicodeString(IPCliente);
                Raiz(InfoXml)->AddChild("TimeoutSesion")->Text = Utileria::LeeIniConfig(NombreSeccionGeneral,NombreValorTimeoutSesion,ValorDefectoTimeoutSesion);

                if(Cfdi.Pos(EsquemaCancelCFDI))
                {
                   DatosCfdi.TipoDoc = IdComprobanteCFDI;
                }
                else{
                     DatosCfdi.TipoDoc = IdDocumentoRetenciones;
                }
                DatosCfdi.XmlInfo= InfoXml;
                Parametros = new TList();
                _DebugLog(strLog+"Antes de ejecutar P.Almacenado ValidacionesPreviasCancelaciones64 en bd administrativa", G8_TOKEN_CANCELACION);
                Bd::AgregaParametro(Parametros, "@Cfdi", DatosCfdi.Cfdi->XML->Text, TDBXDataTypes::WideStringType);
                Bd::AgregaParametro(Parametros, "@Info", DatosCfdi.XmlInfo->XML->Text, TDBXDataTypes::WideStringType);
                Bd::AgregaParametro(Parametros, L"@Resultado", "0", TDBXDataTypes::Int32Type, TDBXParameterDirections::OutParameter);
                Bd::AgregaParametro(Parametros, "@tipoDoc", DatosCfdi.TipoDoc, TDBXDataTypes::Int32Type);
                CdsResultado = new TClientDataSet(NULL);
                /*<!JRMC 25/04/2019 se cambia el nombre del procedimeinto almacenado de la versión a 64 bits para no causar conflicto de códigos en la versión a 32 bits*/
                Resultado = Bd::EjecutaProcedimiento("ValidacionesPreviasCancelaciones64", Parametros,"","","","","","", CdsResultado);

                _LogIntermedio(strLog+"Despues de ejecutar P.Almacenado ValidacionesPreviasCancelaciones64 en bd administrativa "+IntToStr(Resultado), G8_TOKEN_CANCELACION);
                switch ( Resultado )
                {
                    case SESION_CADUCADA:
                        throw ExceptionServidor(ErrorSesionCaducada, MODULO_CFDIS);
                        break;
                    case SESION_INCORRECTA:
                        throw ExceptionServidor(ErrorSesionIncorrecta, MODULO_CFDIS);
                        break;
                    case ERROR_RFCCANCELACIONINVALIDO:
                        throw ExceptionServidor(ErrorRFCCancelacionInvalido, MODULO_CFDIS);
                        break;
                    case ERROR_CONTRASENAINCORRECTA:
                        throw ExceptionServidor(ErrorContraseniaIncorrecta, MODULO_CFDIS);
                        break;
                    case ERROR_USUARIONOEXISTE:
                        throw ExceptionServidor(ErrorUsuarioNoExiste, MODULO_CFDIS);
                        break;
                       // break;
                       // break;
                    case ERROR_SISTEMA_PIRATA:
                       throw ExceptionServidor( StringReplace(ErrorSeriePirata, L"%RFC", DatosCfdi.RfcEmisor, TReplaceFlags() << rfReplaceAll),ERROR_SISTEMA_PIRATA, MODULO_SESION);
                       break;
                    case ERROR_USUARIOSINCONTRATOCANCEL:
                        throw ExceptionServidor(ErrorUsuarioSinContratoCancel, MODULO_CFDIS);
                        break;
                    case ERROR_USUARIOSINULTIMOCONTRATO:
                        throw ExceptionServidor(ErrorUsuarioSinUltimoContrato, MODULO_CFDIS);
                        break;
                    //default:
                       //	throw ExceptionServidor(ErrorGenericoTimbrado, MODULO_CFDIS);
                }

            }
            catch(Exception &e)
            {
            _ErrorLog(strLog+"Inicio del catch de la funcion ValidacionesPreviasCancelaciones: CodigoError="+IntToStr(e.HelpContext), G8_TOKEN_CANCELACION);
                throw Exception(e.Message,e.HelpContext);
            }

        }__finally{

          if ( Parametros )
                {
                    Bd::BorraParametros(Parametros);
                    delete Parametros;
                    Parametros = NULL;
                }
                if ( CdsResultado )
                {
                    if ( !CdsResultado->IsEmpty() )
                    {
                        CdsResultado->First();
                        DatosCfdi.IdPAC= CdsResultado->Fields->Fields[0]->AsInteger ;
                        DatosCfdi.IdEmisor=CdsResultado->Fields->Fields[1]->AsInteger;
                        DatosCfdi.esFacturaMovil=CdsResultado->Fields->Fields[2]->AsInteger;
                        DatosCfdi.RfcEmisor= CdsResultado->Fields->Fields[3]->AsString;
                        DatosCfdi.TipoSesion  =CdsResultado->Fields->Fields[4]->AsInteger;
                        DatosCfdi.IdSerie  =CdsResultado->Fields->Fields[5]->AsInteger;
                        DatosCfdi.Uuid = Raiz(DatosCfdi.Cfdi)->Hijo("Folios")->Hijo("UUID")->Text;
                        delete CdsResultado;
                        CdsResultado = NULL;
                    }else
                    	throw ExceptionServidor(ErrorGenericoTimbrado, MODULO_CFDIS);

                }
                /*<! Inicia JRMC 0504/2019 se agrega por CC-CFDI-1073 para reemplazar por un idDefault cuando el documento entrante es una nómina o una retención */
               if( DatosCfdi.strXmlDoc64.IsEmpty() ){
                    _DebugLog(strLog+"ID_PAC obtenido: "+IntToStr(DatosCfdi.IdPAC), G8_TOKEN_CANCELACION);
                	DatosCfdi.IdPAC = StrToIntDef(Utileria::LeeIniConfig(NombreSeccionGeneral, NombreSeccionPacCancelDefecto, L"7"),7);
                    _DebugLog(strLog+" Se reemplaza el ID_PAC obtenido por el IdPacDefault= "+IntToStr(DatosCfdi.IdPAC)+" de la Seccion General etiqueta PacDefaultCancel para funcion antigua de cancelacion", G8_TOKEN_CANCELACION);
               }
               /*<! Termina JRMC 0504/2019 se agrega por CC-CFDI-1073 para reemplazar por un idDefault cuando el documento entrante es una nómina o una retención */
                strTipo= DatosCfdi.TipoSesion==1 ?   "PAQUETE" : "RENTA";
             _InformationalLog(strLog+"Fin de la funcion ValidacionesPreviasCancelaciones: IdPAC= "+IntToStr(DatosCfdi.IdPAC)+" IdEmisor= "+IntToStr(DatosCfdi.IdEmisor)+" EsFactureMovil= "+IntToStr(DatosCfdi.esFacturaMovil)+" RFCEmisor= "+DatosCfdi.RfcEmisor+" tipoSesion= "+IntToStr(DatosCfdi.TipoSesion)+"("+strTipo+") IdSerie= "+IntToStr(DatosCfdi.IdSerie), G8_TOKEN_CANCELACION);

        }
        /*<!Termina JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
}
//-----------------------------------------------------------------------------
/*!
* \brief      Función que permite agregar al CFDI  a la tabla CFDI_CANCELACION en caso de que no exista  o devolverlo en caso de que si exista
* \author     Programador win A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]  Cfdi: cadena que contiene el XML con los datos del CFDI a cancelar
* @param[in]  Info: cadena que contiene el XML con información de configuración del sistema y datos del cliente
* @param[in]  &DatosCfdi:  objeto de la estructura de datos que   permite guardar la información  recuperada del XML Cfdi
* @param[in]  &CfdiComprometido: guarda verdadero si comprometio el CFDI o false sino se comprometio
* @param[in]  &objDatosConexion: objeto que contiene los datos de la base de datos a a conectarse
* @param[out]
*/
void Cfdis::obtenerOComprometerCancelacion(UnicodeString Cfdi, UnicodeString Info, TDatosDocumento &DatosCfdi, bool &CfdiComprometido,TDatosDocumento &objDatosConexion)
{
    _InformationalLog(strLog+"Inicio de la funcion obtenerOComprometerCancelacion", G8_TOKEN_CANCELACION);
	TClientDataSet *CdsResultado = NULL;
	TList *Parametros = NULL;
	int Resultado     = 0;
    /*<!JRMC 10/12/2018 Inicio se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
    try{
         Parametros = new TList();
         CdsResultado = new TClientDataSet(NULL);
        try
        {
            Bd::AgregaParametro(Parametros, "@Cfdi", DatosCfdi.Cfdi->XML->Text, TDBXDataTypes::WideStringType);
            Bd::AgregaParametro(Parametros, "@Info", DatosCfdi.XmlInfo->XML->Text, TDBXDataTypes::WideStringType);
            Bd::AgregaParametro(Parametros, "@IdPac",DatosCfdi.IdPAC, TDBXDataTypes::Int32Type);
            Bd::AgregaParametro(Parametros, "@ID_USUARIO", DatosCfdi.IdEmisor, TDBXDataTypes::Int32Type);
            Bd::AgregaParametro(Parametros, "@IdNumSerie", DatosCfdi.IdSerie, TDBXDataTypes::Int32Type);
            Bd::AgregaParametro(Parametros, L"@Resultado", "0", TDBXDataTypes::Int32Type, TDBXParameterDirections::OutParameter);
            /*<!JRMC 10/12/2018 se obtmiza código para no repetir codigo inecesario*/
            if(DatosCfdi.TipoDoc==IdComprobanteCFDI)
            {
            	_DebugLog(strLog+"Antes de ejecutar el P. Alamcenado ProcesaPeticionDeCancelacion", G8_TOKEN_CANCELACION);
                Resultado=Bd::EjecutaProcedimiento("ProcesaPeticionDeCancelacion", Parametros,objDatosConexion.Alias,objDatosConexion.Host,objDatosConexion.UsuarioBD,objDatosConexion.Password,objDatosConexion.nombreBD,objDatosConexion.MaxConnections, CdsResultado);
                _DebugLog(strLog+"Despues de ejecutar el P. Alamcenado ProcesaPeticionDeCancelacion: Resultado= "+IntToStr(Resultado), G8_TOKEN_CANCELACION);
            }
            else
            {
                  _DebugLog(strLog+"Antes de ejecutar el P. Alamcenado ProcesaPeticionDeCancelacionRetenciones", G8_TOKEN_CANCELACION);
                  Resultado=Bd::EjecutaProcedimiento("ProcesaPeticionDeCancelacionRetenciones", Parametros,objDatosConexion.Alias,objDatosConexion.Host,objDatosConexion.UsuarioBD,objDatosConexion.Password,objDatosConexion.nombreBD,objDatosConexion.MaxConnections, CdsResultado);
                  _DebugLog(strLog+"Despues de ejecutar el P. Alamcenado ProcesaPeticionDeCancelacionRetenciones: Resultado= "+IntToStr(Resultado), G8_TOKEN_CANCELACION);
            }


            switch ( Resultado )
            {
                case ERROR_FORMATOXMLINVALIDO:
                    throw ExceptionServidor(ErrorFormatoXMLInvalido, MODULO_CFDIS);
                case ERROR_CFDIENPROCESO:
                    throw ExceptionServidor(ErrorCfdiEnProcesoCancelacion, MODULO_CFDIS);
            }

        }
        catch(Exception &e)
        {
            _InformationalLog(strLog+"Inicio del catch de la funcion obtenerOComprometerCancelacion", G8_TOKEN_CANCELACION);
            throw Exception(e.Message,e.HelpContext);
        }
    }__finally{
        if ( Parametros )
            {
                Bd::BorraParametros(Parametros);
                delete Parametros;
                Parametros=NULL;
            }
            if ( CdsResultado )
            {
                 if ( !CdsResultado->IsEmpty() )
                    {
                        CdsResultado->First();
                        DatosCfdi.Estado            = CdsResultado->Fields->Fields[0]->AsInteger;
                        CfdiComprometido            = CdsResultado->Fields->Fields[1]->AsBoolean;
                        DatosCfdi.Acuse             = CdsResultado->Fields->Fields[2]->AsString;
                        delete CdsResultado;
                        CdsResultado = NULL;
                    }
            }
        _InformationalLog(strLog+"Fin de la funcion obtenerOComprometerCancelacion: Estado= "+IntToStr(DatosCfdi.Estado), G8_TOKEN_CANCELACION);
    }

    /*<!JRMC 10/12/2018 Termina se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/

}
//---------------------------------------------------------------------------
/*!
* \brief      Función que invoca al procedimiento almacenado que compromete un paquete de folios o una serie en renta.
* \author     Programador win A Josué Rodrigo Martín Canto
* \version    5.0.0.0
* \date       21/03/2018
* @param[in]  UnicodeString Cfdi, UnicodeString Info, TDatosDocumento &DatosCfdi
* @param[out]
*/
void Cfdis::ComprometePaqueteCancelaciones(TDatosDocumento &DatosCfdi,bool &PaqueteComprometido)
{
    _InformationalLog(strLog+"Inicio de la funcion ComprometePaqueteCancelaciones", G8_TOKEN_CANCELACION);
	TClientDataSet *CdsResultado = NULL;
	TList *Parametros = NULL;
	int Resultado=0;
    /*<!JRMC 10/12/2018 Inicia se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
    try{
        try{
            Parametros   = new TList();
            CdsResultado = new TClientDataSet(NULL);
            _DebugLog(strLog+"Antes de ejecutar el P. Almacenado ComprometePaqueteCancelaciones", G8_TOKEN_CANCELACION);
            Bd::AgregaParametro(Parametros, "@Estado",DatosCfdi.Estado, TDBXDataTypes::Int32Type);
            Bd::AgregaParametro(Parametros, "@TipoSesion",DatosCfdi.TipoSesion, TDBXDataTypes::Int32Type);
            Bd::AgregaParametro(Parametros, "@RfcEmisor", DatosCfdi.RfcEmisor, TDBXDataTypes::WideStringType);
            Bd::AgregaParametro(Parametros, "@IdUsuario",DatosCfdi.IdEmisor, TDBXDataTypes::Int32Type);
            Bd::AgregaParametro(Parametros, "@EsFactureMovil",DatosCfdi.esFacturaMovil, TDBXDataTypes::Int32Type);
            Bd::AgregaParametro(Parametros, "@IdNumSerie", DatosCfdi.IdSerie, TDBXDataTypes::Int32Type);
            Bd::AgregaParametro(Parametros, L"@Resultado", "0", TDBXDataTypes::Int32Type, TDBXParameterDirections::OutParameter);
            Resultado=Bd::EjecutaProcedimiento("ComprometerPaqueteCancelaciones", Parametros,"","","","","","", CdsResultado);
           _DebugLog(strLog+"Despues de ejecutar el P. Almacenado ComprometePaqueteCancelaciones: Resultado= "+IntToStr(Resultado), G8_TOKEN_CANCELACION);
             if ( !CdsResultado->IsEmpty() )
                {
                    CdsResultado->First();
                    if(DatosCfdi.Estado==1)
                    {
                        if(CdsResultado->Fields->Fields[0]->AsInteger==-1){
                         PaqueteComprometido= true;
                        } else{
                            Resultado=CdsResultado->Fields->Fields[0]->AsInteger;
                        }
                    }
                }
                else
                    throw ExceptionServidor(ErrorGenericoTimbrado, MODULO_CFDIS);

             switch ( Resultado )
            {
                case ERROR_FOLIOSAGOTADOS:
                    throw ExceptionServidor(ErrorFoliosAgotados, MODULO_CFDIS);
                    break;
                case ERROR_SERIE_RENTA_CADUCADA:
                   throw ExceptionServidor(ErrorSerieRentaCaducada, MODULO_CFDIS);
                   break;
            }
        _InformationalLog(strLog+"Fin de la funcion ComprometePaqueteCancelaciones: Resultado="+IntToStr(Resultado), G8_TOKEN_CANCELACION);
        }
        catch(Exception &e){

           throw Exception(e.Message,e.HelpContext);
        }
    }__finally{
            if ( Parametros )
            {
             	Bd::BorraParametros(Parametros);
                delete Parametros;
                Parametros=NULL;
            }
                if ( CdsResultado )
            {
                delete CdsResultado;
                CdsResultado = NULL;
            }
    }
     /*<!JRMC 10/12/2018 Termina se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/

}
//---------------------------------------------------------------------------
/*!
* \brief      Función principal la cual contiene toda la lógica del proceso del servicio de cancelación
* \author     Programador A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]  Cfdi: cadena que contiene el XML con los datos del CFDI a cancelar
* @param[in]  Info: cadena que contiene el XML con información de configuración del sistema y datos del cliente
* @param[out]  UnicodeString: cadena que contiene un XML de error o de exito de la cancelación
*/
UnicodeString Cfdis::EjecutaCancelarCFDI(UnicodeString Cfdi, UnicodeString Info, bool esRetencion)
{
	_LogEntrada(G8_TOKEN_CANCELACION);
    strLog= "_"+IntToStr((int)GetCurrentThreadId())+"_"+UnicodeString(IdPeticion)+" | ";
    _InformationalLog(strLog+"Inicio de la funcion EjecutaCancelarCFDI(Antiguo Esquema Cancelacion)", G8_TOKEN_CANCELACION);
	bool PaqueteComprometido = false, CfdiComprometido = false;
	_di_IXMLDocument Resultado = NULL;
	TDatosDocumento objDatosConexion;
	UnicodeString strSistemaParaCancelacion = "",  strEmisor="";
	UnicodeString MensajeResultado, Acuse;
	int CodigoResultado, iReinstalableParaCancelacion=-1;
     bool NuevoEsquema = false; /*<! JRMC 10/09/2018 Se agrega variable para rescatar valor de etiqueta en el archivo de configuración */
	TDatosDocumento DatosCfdi;
    DatosCfdi.IdEmisor=0;
	DatosCfdi.Acuse = "";
	DatosCfdi.IdProceso = ProcesoCancelar;
	DatosCfdi.ErrorSinUUID = false;
	DatosCfdi.CfdiComoCadena = Cfdi;
	_di_IXMLDocument InfoSistema = Utileria::dameNuevoDocXML(Info);
	try { strSistemaParaCancelacion = Raiz(InfoSistema)->Hijo("SISTEMA")->Text; } catch(...) {}
	try {iReinstalableParaCancelacion = (Raiz(InfoSistema)->Hijo("REINSTALABLE")->Text).ToInt(); } catch(...) {}
	try {DatosCfdi.RfcEmisor  = Raiz(InfoSistema)->Hijo("USUARIO")->Text;} catch(...) {}
    InfoSistema->Active=false;
	InfoSistema = NULL;

    strLog+=DatosCfdi.CfdiComoCadena.SubString(DatosCfdi.CfdiComoCadena.Pos("<UUID>")+6,DatosCfdi.CfdiComoCadena.Pos("</UUID>")-(DatosCfdi.CfdiComoCadena.Pos("<UUID>")+6))+" | ";
    _InformationalLog(strLog+"keyCancel Sistema= "+strSistemaParaCancelacion+" Reinstalable= "+IntToStr(iReinstalableParaCancelacion)+" RFCEmisor= "+DatosCfdi.RfcEmisor+" Servidor= "+ Utileria::LeeIniConfig(NombreSeccionGeneral,"Servidor","0"), G8_TOKEN_CANCELACION);
    /*<!Inicia JRMC 11/09/2018 se agrega por CC-CFDI-645  para validar si esta activo el nuevo esquema de cancelación  */
	try{
	   NuevoEsquema = StrToBool(Utileria::LeeIniConfig("GENERAL","NuevoEsquemaCancel","0"));
	}
	catch(Exception &e){
		_WarningLog(strLog+"Error al rescatar valor de etiqueta"+e.Message,G8_TOKEN_CANCELACION);
	}
    /*<! Termina JRMC 11/09/2018 se agrega por CC-CFDI-645  para validar si esta activo el nuevo esquema de cancelación */
    /*<!Inicia JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
    try{
        try
        {

            int cuantos = TSeleccionarPacParaCancelacion::CuantosThreadHay();
            _InformationalLog(strLog+"Cuantos threads ocupados hay:  "+UnicodeString(cuantos), G8_TOKEN_CANCELACION);
            if(cuantos >= StrToIntDef(Utileria::LeeIniConfig(NombreSeccionGeneral,NumThreadsMaximo, IntToStr(ValorDefectoNumThreadsMaximo)), ValorDefectoNumThreadsMaximo))
                throw ExceptionServidor(ErrorCancelacionNoActiva, MODULO_CFDIS);

            if((Cfdi.Pos(EsquemaCancelRetencion)) && (Utileria::LeeIniConfig(NombreSeccionGeneral, NombreLlaveActivarCancelacionesRtc, L"1") == L"0")  )
                throw ExceptionServidor(ErrorCancelacionRtcNoActivaRtc, MODULO_CFDIS);

            if ( Utileria::LeeIniConfig(NombreSeccionGeneral, NombreLlaveActivarCancelaciones, L"1") == L"0" )
                throw ExceptionServidor(ErrorCancelacionNoActiva, MODULO_CFDIS);

            if (!ValidaSistema(UpperCase(strSistemaParaCancelacion) , iReinstalableParaCancelacion,1))
                throw ExceptionServidor(ErrorSistemaCancelacionNoActiva, MODULO_CFDIS);
            _InformationalLog(strLog+"Procesando petición de cancelacion "+strEmisor, G8_TOKEN_CANCELACION);
            _IniciaTicks;

            /*!< Inicia JRMC 09/04/2019  se sube condicion por CC-CFDI-1088 para finalizar el proceso de cancelacion cuando el documento no es una nómina*/
            /*!< JRMC 18/01/2019 Se cambia condición para permitir cancelación de NOI de forma Masiva (NOIWIN a solo NOI)*/
            if(strSistemaParaCancelacion.UpperCase().Pos("NOI")>0 || esRetencion == true || Cfdi.Pos(EsquemaCancelRetencion )>0  || NuevoEsquema == false) //JRMC 10/09/2018 Se agrega nueva condición para validar si se muestra mensaje cuando bandera está activada
            {
                DatosCfdi.EsNOI = true;
            }
        	else{
                DatosCfdi.EsNOI = false;
                _DebugLog(strLog+"Se  finaliza el proceso de cancelacion porque el documento no es una nomina o una retencion", G8_TOKEN_CANCELACION);
                throw ExceptionServidor(ErrorSistemaCancelacionNoActiva, MODULO_CFDIS);
            }
            /*!< Termina JRMC 09/04/2019  se sube condicion por CC-CFDI-1088 para finalizar el proceso de cancelacion cuando el documento no es una nómina*/
            ValidacionesPreviasCancelaciones(Cfdi,Info,DatosCfdi);
            TseleccionarApuntadoresBD::obtenerBD(DatosCfdi.IdEmisor,objDatosConexion);
            _InformationalLog(strLog+"Bd a conectar= "+objDatosConexion.nombreBD+" host= "+objDatosConexion.Host, G8_TOKEN_CANCELACION);
            obtenerOComprometerCancelacion(Cfdi, Info, DatosCfdi, CfdiComprometido,objDatosConexion);
            ComprometePaqueteCancelaciones(DatosCfdi,PaqueteComprometido);
            bool HuboTimeout = false;
            /*<! Inicia JRMC 31/08/2018 Se agrega condición para no consumir siempre el servicio del PAC y regresar el mensaje de acuerdo al sistema.*/
            if(DatosCfdi.Estado != EstCfdiTimbrado && DatosCfdi.Estado != EstNecesitaAutorizacion)
            {
                try
                {
                  Cancela(DatosCfdi, Resultado, MensajeResultado, CodigoResultado, Acuse,objDatosConexion);
                }
                catch(Exception &e)
                {
                    if ( e.HelpContext == ERROR_INTERNET_TIMEOUT )
                    {
                        _NoticeLog(strLog+"Error de timeout en PAC " + IntToStr(DatosCfdi.IdPAC)+" Emisor: "+strEmisor, G8_TOKEN_CANCELACION);
                        HuboTimeout = true;
                        MensajeResultado = ErrorUUIDVacio;
                        CodigoResultado  = ERROR_UUID_VACIO;
                    }
                    else
                        throw Exception(e.Message,e.HelpContext);
                }
                DatosCfdi.Acuse = Acuse;
                TEstadoCfdi NuevoEstado = EstCfdiIndefinido;
                bool EsNOI = true;
               
                /*!< JRMC 18/01/2019 Se cambia condición para permitir cancelación de NOI de forma Masiva (NOIWIN a solo NOI)*/
			    if(strSistemaParaCancelacion.UpperCase().Pos("NOI")>0 || esRetencion == true || Cfdi.Pos(EsquemaCancelRetencion)>0 || NuevoEsquema == false)
                {
                    NuevoEstado = HuboTimeout ? EstCfdiPendiente : EstCfdiTimbrado;  //existe el timeout si existe el estado pendiente(3) sino el estado es timbrado (4)
                }
                else{
                    NuevoEstado = HuboTimeout ? EstCfdiPendiente : EstNecesitaAutorizacion;
                    EsNOI = false;
                }
                /*<! Inicia JRMC 31/08/2018 Se agrega condición para determinar que estado se va a guardar en la BD.*/
                TOperacionPaquete OpPaquete = DatosCfdi.Estado == EstCfdiPendiente ? OpPaqLiberar : OpPaqDecrementarYLiberar;
                _DebugLog(strLog+"Cencelado exitoso se procede a Actualizar con los siguientes datos: opPaquete= "+IntToStr(OpPaquete)+" opCfdi= 3(Actualizar) NuevoEstado= "+IntToStr(NuevoEstado), G8_TOKEN_CANCELACION);
                DatosCfdi.DuracionProceso = _TiempoTotal;
                DatosCfdi.Sistema =  UpperCase(strSistemaParaCancelacion) + ".EXE";
                Actualiza(DatosCfdi, NuevoEstado, OpCfdiActualizaCancelado, OpPaquete,objDatosConexion);
                if(!EsNOI && !HuboTimeout)
                {
                    //MensajeResultado = "Estimado usuario hemos enviado tu petición de cancelación al SAT pero por los cambios implementados en el nuevo esquema de cancelación es probable que la cancelación no proceda fiscalmente, te recomendamos ampliamente NO enviar a cancelar documentos hasta que actualices tu sistema con la última versión.";
                    MensajeResultado = "Estimado usuario: Se ha enviado tu petición de cancelación del comprobante. Sin embargo, debido a los cambios que el SAT ha instrumentado en el proceso de cancelación de CFDIs, existe la posibilidad que la cancelación fiscal no proceda. Te recomendamos ampliamente NO enviar a cancelar documentos hasta que actualices tu sistema con la última versión, en la que se efectúa el proceso de cancelación bajo las nuevas condiciones establecidas.";
                    CodigoResultado = 911;
                    DatosCfdi.Acuse = "";
                    _DebugLog(strLog+"EL SISTEMA ES DIFERENTE DE NOI["+strSistemaParaCancelacion+"]",G8_TOKEN_CANCELACION);
                }
               _InformationalLog(strLog+"Fin del proceso de cancelacion exitosamente(Antiguo Esquema cenacelacion ) : Resultado="+Utileria::ErrorCodigoMensaje(MensajeResultado, CodigoResultado, DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError), G8_TOKEN_CANCELACION);
            }else{

                
                /*!< LASE 11/12/2018 Se cambia condición para permitir cancelación de NOI de forma Masiva (NOIWIN a solo NOI)*/
			     if(strSistemaParaCancelacion.UpperCase().Pos("NOI")>0 || esRetencion == true || Cfdi.Pos(EsquemaCancelRetencion)>0 || NuevoEsquema == false)/*<!JRMC 18/01/2019 se agrega para poder cancelar de forma masiva  desde NOI*/
                {
                    MensajeResultado = "Cancelado sin autorización";
                    CodigoResultado = -1;
                    _DebugLog(strLog+"[RECANCELACION]EL SISTEMA ES NOI["+strSistemaParaCancelacion+"] o es una RETENCION",G8_TOKEN_CANCELACION);
                }
                else{
                    //MensajeResultado = "Estimado usuario hemos enviado tu petición de cancelación al SAT pero por los cambios implementados en el nuevo esquema de cancelación es probable que la cancelación no proceda fiscalmente, te recomendamos ampliamente NO enviar a cancelar documentos hasta que actualices tu sistema con la última versión.";
                    MensajeResultado = "Estimado usuario: Se ha enviado tu petición de cancelación del comprobante. Sin embargo, debido a los cambios que el SAT ha instrumentado en el proceso de cancelación de CFDIs, existe la posibilidad que la cancelación fiscal no proceda. Te recomendamos ampliamente NO enviar a cancelar documentos hasta que actualices tu sistema con la última versión, en la que se efectúa el proceso de cancelación bajo las nuevas condiciones establecidas.";
                    CodigoResultado = 911;
                    DatosCfdi.Acuse = "";
                    _DebugLog(strLog+"[RECANCELACION]EL SISTEMA ES DIFERENTE DE NOI["+strSistemaParaCancelacion+"]",G8_TOKEN_CANCELACION);
                }
                _InformationalLog(strLog+"Fin del proceso de cancelacion (Antiguo esquema ) porque esta en estado 4 y ya existe el Acuse: Resultado= "+Utileria::ErrorCodigoMensaje(MensajeResultado, CodigoResultado, DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError), G8_TOKEN_CANCELACION);
            }
            PaqueteComprometido = false;
            return Utileria::ErrorCodigoMensaje(MensajeResultado, CodigoResultado, DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError);
        }
        catch(Exception &e)
        {

            _ErrorLog(strLog+"Inicio del catch Principal de la funcion principal  EjecutaCancelarCFDI(antiguo esquema)", G8_TOKEN_CANCELACION);

            if ( DatosCfdi.IdEmisor != 0 )
                TseleccionarApuntadoresBD::obtenerBD(DatosCfdi.IdEmisor,objDatosConexion);
            if (e.HelpContext == ERROR_DESCONOCIDO)
            {
                e.Message = ErrorGenericoTimbrado;
                _ErrorLog(strLog+"Error desconocido, se cambia el mensaje Por el mensaje del error generico", G8_TOKEN_CANCELACION);
            }

            TOperacionCfdi OpCfdi = CfdiComprometido ? OpCfdiEliminarCancelado : OpCfdiNinguna;
            TOperacionPaquete OpPaquete = PaqueteComprometido ? OpPaqLiberar : OpPaqNinguna;
            if(DatosCfdi.Estado == EstCfdiPendiente)
               OpPaquete= OpPaqIncrementar;

             if ( DatosCfdi.IdEmisor != 0 ){
                _ErrorLog(strLog+"En el catch principal  se procede a Actualizar con los siguientes datos por error en la cancelacion : opPaquete= "+IntToStr(OpPaquete)+" opCfdi= 4(Eliminar) NuevoEstado= "+IntToStr(EstCfdiIndefinido)+" IdCFDI= "+IntToStr(DatosCfdi.Id), G8_TOKEN_CANCELACION);
                Actualiza(DatosCfdi, EstCfdiIndefinido, OpCfdi, OpPaquete,objDatosConexion);
             }

            _ErrorLog(strLog+"Fin del catch principal de la funcion EjecutaCancelarCFDI(Antiguo esquema) : Resultado= "+Utileria::ErrorCodigoMensaje(e.Message, e.HelpContext,DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError), G8_TOKEN_CANCELACION);

            return Utileria::ErrorCodigoMensaje(e.Message, e.HelpContext,DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError);
        }
     }__finally{
       if(DatosCfdi.XmlInfo){
       	 DatosCfdi.XmlInfo->Active=false;
       	 DatosCfdi.XmlInfo=NULL;
       }
       if(DatosCfdi.Cfdi) {
       	  DatosCfdi.Cfdi->Active=false;
      	  DatosCfdi.Cfdi=NULL;
       }
       if(Resultado){
       	Resultado->Active=false;
       	Resultado= NULL;
       }
     }
     /*<!Termina JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
}
//------------------------------------------------------------------------------
/*!
* \brief      Función que inicializa algunas variables de la estructura de estructyara TDatosDocumento en el archivo EstructuraDatos.h las cuales son necesarios durante el proceso de cancelación
* \author     Programador win A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]
* @param[out]
*/
TDatosDocumento::TDatosDocumento()
{
	IdPAC = -1;
	Id = -1;
	DuracionProceso = 0L;
	DuracionPac = 0L;
	DuracionSesionCancela = 0L;
	Estado = EstCfdiIndefinido;
}
//---------------------------------------------------------------------------
/*!
* \brief      Función que construye la estructura de una cadena  con todos los datos necesarios para actualizzar la tabla CFDIS_CANCELACION en la bd de documentos elegido dinamicamente por el idEmisor
* \author     Programador win A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]
* @param[out]  Resultado: cadena que contiene la estructura de un XML
*/
UnicodeString TDatosDocumento::DatosComoXml()
{
	UnicodeString Resultado;
	Resultado += L"<DATOSCFDI>";
	Resultado += 	L"<ID>" + IntToStr(Id) + L"</ID>";
	Resultado +=	L"<SISTEMA>" + Sistema + L"</SISTEMA>";
	Resultado +=	L"<IPORIGEN>" + UnicodeString(IPCliente) + L"</IPORIGEN>";
	Resultado +=	L"<DURACIONPROCESO>" + UIntToStr(DuracionProceso) + L"</DURACIONPROCESO>";
	Resultado +=	L"<DURACIONPAC>" + UIntToStr(DuracionPac) + L"</DURACIONPAC>";
	Resultado +=	L"<DURACIONSESION>" + UIntToStr(DuracionSesionCancela) + L"</DURACIONSESION>";
	Resultado +=	L"<UUID>" + Uuid + L"</UUID>";
	Resultado +=	L"<IDEMISOR>" + IntToStr(IdEmisor) + L"</IDEMISOR>";
	Resultado +=	L"<FECHACANCELACION>" + FechaCancelacion + L"</FECHACANCELACION>";
	Resultado +=	L"<RFCEMISOR>" + HTMLEscape(RfcEmisor) + L"</RFCEMISOR>";
	Resultado +=	L"<TIPO_DOC>" + IntToStr(TipoDoc) + L"</TIPO_DOC>";
	Resultado +=	L"<ENVIO_CFDIS_SINCRONO>" + (Utileria::LeeIniConfig(NombreSeccionGeneral,EnvioSincronoCfdisEspacio, IntToStr(ValorDefectoEnvioSincronoCfdisEspacio)))+ L"</ENVIO_CFDIS_SINCRONO>";
	Resultado +=	L"<TIPO_DISPERSION>" + IntToStr(TipoDispersion) + L"</TIPO_DISPERSION>";
	Resultado +=	L"<VIGENCIA_DISPERSION>" +VigenciaDispersion + L"</VIGENCIA_DISPERSION>";
	Resultado +=	L"<FECHA>" + Fecha  + L"</FECHA>";
	Resultado +=	L"<DE_ASPEL>" + BoolToStr(CancelDeAspel, true)+ L"</DE_ASPEL>";
	Resultado +=	L"<ID_CANCEL>" + IntToStr(IdCancel)  + L"</ID_CANCEL>";
	Resultado += L"</DATOSCFDI>";
	return Resultado;
}
//---------------------------------------------------------------------------
/*!
* \brief      Verfica que el sistema/reinstalable que solicito la peticion de cancelacion se encuentre activado/preparado para dicha operación.
* \details    Esta función se utiliza para verificar si el sistema que lanzo la peticion se encuentra preparado para realizar la cancelacion de CFDI,
* \details    para ello se consulta de un archivo de configuración ini el nombre y numero de reinstalable de los sistemas que ya pueden cancelar, para
* \details    comparar si el sistema/version que envió la petición se encuentra dentro de dicha lista.
* \author     Programador win A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]  strSistema UnicodeString Nombre del sistema que mando la peticion de cancelacion
* @param[in]  Tipo int Versión del esquema de cancelación que puede ser 1(Versión antigua) o 2(Versión nueva).
* @param[out] bool True si el sistema si se encuentra preparado para la cancelacióm y False en caso de no estarlo
*/
bool Cfdis::ValidaSistema(UnicodeString strSistema, int iReinstalable,int  Tipo)
{
	/*
     * Tipo = 1 (Versión antigua Esquema de cancelación)
     * Tipo = 2 (Versión nueva Esquema de cancelación)
     */
  //_LogEntrada(G8_TOKEN_CANCELACION);
  _InformationalLog(strLog+"Inicio de la funcion ValidaSistema", G8_TOKEN_CANCELACION);
  _DebugLog(strLog+" Sistema= "+UpperCase(strSistema)+" R= "+IntToStr(iReinstalable), G8_TOKEN_CANCELACION);
  int iSistema = 1,iContadorSistemas=0,iReinstalableValido=0;
  UnicodeString strSistemaValido;

  if(Tipo == 1){
	iContadorSistemas = (Utileria::LeeIniConfig(NombreSeccionCancelacion, NombreActivarContadorSistemas, L"1")).ToInt();
     _DebugLog(strLog+"Se obtubo cantidad disponibles= "+IntToStr(iContadorSistemas)+" para antiguo esquema de la Seccion="+NombreSeccionCancelacion, G8_TOKEN_CANCELACION);
  }
  else if(Tipo == 2){
	iContadorSistemas = (Utileria::LeeIniConfig(NombreSeccionCancelacionV2, NombreActivarContadorSistemas, L"1")).ToInt();

  _DebugLog(strLog+"Se obtubo cantidad disponibles= "+IntToStr(iContadorSistemas)+" para nuevo esquema de la Seccion="+NombreSeccionCancelacionV2, G8_TOKEN_CANCELACION);
  }


  if( (strSistema.IsEmpty()) && (iReinstalable==-1) )
	return false;

  while( iSistema<=iContadorSistemas)
  {

     if(Tipo == 1){
		strSistemaValido = Utileria::LeeIniConfig(NombreSeccionCancelacion, NombreLlaveActivarCancelacionesPorSistema+ IntToStr(iSistema), L"Facture20");
		iReinstalableValido = Utileria::LeeIniConfig(NombreSeccionCancelacion, NombreLlaveActivarCancelacionesPorReinstalable+ IntToStr(iSistema), L"0").ToInt();
	 }
	 else if(Tipo == 2){
     	strSistemaValido = Utileria::LeeIniConfig(NombreSeccionCancelacionV2, NombreLlaveActivarCancelacionesPorSistema+ IntToStr(iSistema), L"Facture20");
	 	iReinstalableValido = Utileria::LeeIniConfig(NombreSeccionCancelacionV2, NombreLlaveActivarCancelacionesPorReinstalable+ IntToStr(iSistema), L"0").ToInt();
	 }
     _DebugLog(strLog+"SistemaIni = "+strSistemaValido+" "+IntToStr(iReinstalableValido)+" SistemaRecibido = "+strSistema,G8_TOKEN_CANCELACION);
	 if((strSistema == strSistemaValido) && (iReinstalable >= iReinstalableValido))
	 {
	  _DebugLog(strLog+"El sistema y reinstalable recibido esta activado para cancelaciones "+strSistema+" R"+IntToStr(iReinstalable), G8_TOKEN_CANCELACION);
	   return true;
	 }
	 else
	 {
	   _DebugLog(strLog+"El sistema y reinstalable recibido no esta activado para cancelaciones "+strSistema+" R"+IntToStr(iReinstalable), G8_TOKEN_CANCELACION);
	   iSistema++;
	   if (iSistema>iContadorSistemas)
		 return false;
	 }

  }
  _InformationalLog(strLog+"Fin de la funcion ValidaSistema", G8_TOKEN_CANCELACION);
}
//------------------------------------------------------------------------------
/*!
* \brief      Funcion que realiza la instancia hacia el PAC al cual se le enviara la peticion, para despues invocar a la funcion que ejecuta la cancelacion
* \author     Programador win A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]  TDatosDocumento &DatosCfdi   Para obtener el valor del atributo IdPAC, el cual servira para saber con que pac realizar la instancia
* @param[in]  _di_IXMLDocument &Resultado parametro por referencia en el cual se almacenara la peticion de cancelacion enviada al PAC
* @param[in]  UnicodeString &MensajeResultado parametro por referencia en el cual se almacenara el mensaje de respuesta del PAC
* @param[in]  int &CodigoResultado parametro por referencia en el cual se almacenara el codigo de respuesta del PAC
* @param[out] UnicodeString &Acuse  parametro por referencia en el cual se almacenara el acuse de cancelacion que devuelva el PAC cuando se trate de Mysuite
*/
void Cfdis::EjecutaCancelacion(TDatosDocumento &DatosCfdi, _di_IXMLDocument &Resultado, UnicodeString &MensajeResultado, int &CodigoResultado, UnicodeString &Acuse)
{
	_InformationalLog(strLog+"Inicio de la funcion EjecutaCancelacion", G8_TOKEN_CANCELACION);
	TPac *Pac = NULL;

	try
	{
		switch ( DatosCfdi.IdPAC )
		{
		   /*case IdPACMasterEdi:
				Pac = new TMasterEdi(DatosCfdi.CfdiComoCadena);
			   	break;  */
			case IdPACMySuite:
				 Pac = new TMySuite(DatosCfdi.CfdiComoCadena, DatosCfdi.strXmlDoc64);
				break;
			/*case IdPACExpideTuFactura:
				Pac = new TXPDCancela(DatosCfdi.CfdiComoCadena);
				break; */
			case IdPACComercioDigital:
			   	Pac = new TComercioDigitalCancela(DatosCfdi.CfdiComoCadena,DatosCfdi.strXmlDoc64);
				break;
			case IdPACAspel:
				Pac = new TAspelPac(DatosCfdi.CfdiComoCadena,IdPACAspel,DatosCfdi.strXmlDoc64);
				break;
			case IdPACTSP:
				Pac = (TPac *)new TAspelPac(DatosCfdi.CfdiComoCadena, IdPACTSP,DatosCfdi.strXmlDoc64);
				break;
			default:
				throw ExceptionServidor(ErrorGenericoTimbrado, MODULO_CFDIS);
		}

		Pac->CancelaCfdi(Resultado, Acuse);
		DatosCfdi.DuracionPac = Pac->Duracion;
		MensajeResultado = Pac->MensajeError;
		CodigoResultado = Pac->CodigoError;
        DatosCfdi.strCancelDetalleError= Pac->strDetalleError;
		delete Pac;
		Pac = NULL;
        _InformationalLog(strLog+"Fin de la funcion EjecutaCancelacion", G8_TOKEN_CANCELACION);
	}
	catch(Exception &e)
	{
        _ErrorLog(strLog+"Inicio del catch de la funcion EjecutaCancelacion", G8_TOKEN_CANCELACION);
        DatosCfdi.Acuse=Acuse;/*<!JRMC 22/08/2018 Se agrega por RQ-CFDI-601 para obtener el acuse cuando existe un error del nuevo esquema de cancelación y es necesario retornar el acuse*/
        DatosCfdi.strCancelDetalleError= Pac->strDetalleError;
		if ( Pac )
			delete Pac;
        _ErrorLog(strLog+"Inicio del catch de la funcion EjecutaCancelacion", G8_TOKEN_CANCELACION);
		throw Exception(e.Message,e.HelpContext);
	}
}
//------------------------------------------------------------------------------
/*!
* \brief      Invoca a la funcion  seleccionarMejorPuntaje de  TSeleccionarPacParaCancelacion para obtener el pac con el cual se enviara a cancelar la peticion
* \author     Programador win A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]  TDatosDocumento &DatosCfdi para enviar el IdPac almacenado en el atributo IdPAC y guardar en ese mismo atributo la respuesta que nos devuelva la funcion
* @param[out]
*/
void Cfdis::EligePacParaCancelar(TDatosDocumento &DatosCfdi)
{
    try{
        _InformationalLog(strLog+"Inicio de la funcion EligePacParaCancelar", G8_TOKEN_CANCELACION);
        if (DatosCfdi.TipoDoc==IdComprobanteCFDI){
            if ( DatosCfdi.Estado == EstCfdiNuevo )
                DatosCfdi.IdPAC = TSeleccionarPacParaCancelacion::seleccionarMejorPuntaje(DatosCfdi.IdPAC);
        }else if(DatosCfdi.TipoDoc==IdDocumentoRetenciones){
            if ( DatosCfdi.Estado == EstCfdiNuevo )
            DatosCfdi.IdPAC = TSeleccionarPacParaCancelaRetencion::seleccionarMejorPuntaje(DatosCfdi.IdPAC);
        }
        _InformationalLog(strLog+"Fin de la funcion EligePacParaCancelar: PacElegido="+IntToStr(DatosCfdi.IdPAC), G8_TOKEN_CANCELACION);
    }
    catch(Exception &e)
    {
        DatosCfdi.IdPAC = StrToIntDef(Utileria::LeeIniConfig(NombreSeccionGeneral, NombreSeccionPacCancelDefecto, L"7"),7);
        _WarningLog(strLog+"Inicio del catch de la funcion EligePac, Se obtiene la etiqueta  PacDefaultCancel="+DatosCfdi.IdPAC+"  de la sesion [General]", G8_TOKEN_CANCELACION);
    }
}
//------------------------------------------------------------------------------
/*!
* \brief      Invoca a la funcion  actualizarEstadisticas de  TSeleccionarPacParaCancelacion para indicar cual fue la duracion del PAC en procesar una peticion de cancelacion
* \author     Programador win  A. Josué Rodrigo Martín canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]  const TDatosDocumento DatosCfdi para obtener los valores de los atributos IdPAC y DuracionPac
* @param[out]
*/
void Cfdis::ActualizaInfoSeleccionPACParaCancelar(const TDatosDocumento DatosCfdi)
{
    _InformationalLog(strLog+"Inicio de la funcion ActualizaInfoSelecionPAcParaCancelacion", G8_TOKEN_CANCELACION);
    if (DatosCfdi.TipoDoc==IdComprobanteCFDI){
        if ( DatosCfdi.Estado == EstCfdiNuevo )
            TSeleccionarPacParaCancelacion::actualizarEstadisticas(DatosCfdi.IdPAC, DatosCfdi.DuracionPac);
    }else if(DatosCfdi.TipoDoc==IdDocumentoRetenciones){
       	if ( DatosCfdi.Estado == EstCfdiNuevo )
		TSeleccionarPacParaCancelaRetencion::actualizarEstadisticas(DatosCfdi.IdPAC, DatosCfdi.DuracionPac);
    }
    _InformationalLog(strLog+"Fin de la funcion ActualizaInfoSelecionPAcParaCancelacion", G8_TOKEN_CANCELACION);

}
//------------------------------------------------------------------------------
/*!
* \brief      Funcion que modifica el ID del PAC con el que se realizara la cancelacion de un UUID
* \author     Programador win A. Josué Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       20-03-2018
* @param[in]  TDatosDocumento &DatosCfdi para obtener el valor del atributo IdPAC con el cual se realizara la cancelacion
* @param[out]
*/
void Cfdis::ActualizaPacDeCfdiParaCancelar(TDatosDocumento &DatosCfdi,TDatosDocumento &objDatosConexion)
{

     _InformationalLog(strLog+"Inicio de la funcion ActualizaPacDeCfdiParaCancelar", G8_TOKEN_CANCELACION);
	if (DatosCfdi.TipoDoc==IdComprobanteCFDI){
		Bd::EjecutaSQL(L"UPDATE CANCELACIONES SET ID_PAC = " + IntToStr(DatosCfdi.IdPAC) + L" WHERE UUID = " + QuotedStr(DatosCfdi.Uuid),objDatosConexion.Alias,objDatosConexion.Host,objDatosConexion.UsuarioBD,objDatosConexion.Password,objDatosConexion.nombreBD,objDatosConexion.MaxConnections);     /*!< NFQM 14/Jun/2012 */
        _InformationalLog(strLog+"Se actualiza el campo ID_PAC de la tabla CANCELACIONES en la bd documentos="+objDatosConexion.nombreBD+"  Ip="+objDatosConexion.Host, G8_TOKEN_CANCELACION);
	}else  {
		Bd::EjecutaSQL(L"UPDATE CANCELA_RETENCIONES SET ID_PAC = " + IntToStr(DatosCfdi.IdPAC) + L" WHERE UUID = " + QuotedStr(DatosCfdi.Uuid),objDatosConexion.Alias,objDatosConexion.Host,objDatosConexion.UsuarioBD,objDatosConexion.Password,objDatosConexion.nombreBD,objDatosConexion.MaxConnections);
        _InformationalLog(strLog+"Se actualiza el campo ID_PAC de la tabla CANCELA_RETENCIONES en la bd documentos="+objDatosConexion.nombreBD+"  Ip="+objDatosConexion.Host, G8_TOKEN_CANCELACION);
      }
    _InformationalLog(strLog+"Fin de la funcion ActualizaPacDeCfdiParaCancelar", G8_TOKEN_CANCELACION);
}

//--------------------------------------------------------------------------
/*!
* \brief      Función que permite realizar el proceso de validación  y envio de cancelación con el nuevo esquema  del SAT
* \author     Programador A. Josue Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       17-Agosto-2018
* @param[in]  astrCFDI contiene el UUID en una estructura del documento de cancelar
* @param[in]  astrXMLConfig contiene los datos del usuario el cual envia la petición
* @param[in]  astrXMLdoc contiene el CFDI del UUID a cancelar   codigicado a 64 bits
* @param[out]  strResultado xml que contiene el resultado dependiendo de la operacion solicitada
*/
//---------------------------------------------------------------------------
UnicodeString Cfdis::EjecutaCancelarCFDIV2(UnicodeString strCfdiCancelacion, UnicodeString Info,UnicodeString strXmlDoc64)
{

    strLog= "_"+IntToStr((int)GetCurrentThreadId())+"_"+UnicodeString(IdPeticion)+" | ";
    _InformationalLog(strLog+"Inicio de la funcion EjecutaCancelarCFDIV2(Nuevo Esquema Cancelacion)", G8_TOKEN_CANCELACION);
	bool PaqueteComprometido = false, CfdiComprometido = false;
    _di_IXMLDocument Resultado = NULL;
	TDatosDocumento objDatosConexion;
	UnicodeString strSistemaParaCancelacion = "",  strEmisor="";
	UnicodeString MensajeResultado, Acuse;
	int CodigoResultado, iReinstalableParaCancelacion=-1;
	TDatosDocumento DatosCfdi;
	DatosCfdi.IdEmisor=0;
	DatosCfdi.Acuse = "";
	DatosCfdi.IdProceso = ProcesoCancelar;
	DatosCfdi.ErrorSinUUID = false;
	DatosCfdi.CfdiComoCadena = strCfdiCancelacion;
	DatosCfdi.strXmlDoc64="";
	DatosCfdi.strXmlDoc64=strXmlDoc64;

	_di_IXMLDocument InfoSistema = Utileria::dameNuevoDocXML(Info);
	try { strSistemaParaCancelacion = Raiz(InfoSistema)->Hijo("SISTEMA")->Text; } catch(...) {}
	try {iReinstalableParaCancelacion = (Raiz(InfoSistema)->Hijo("REINSTALABLE")->Text).ToInt(); } catch(...) {}
	try {DatosCfdi.RfcEmisor  = Raiz(InfoSistema)->Hijo("USUARIO")->Text;} catch(...) {}
    InfoSistema->Active=false;
	InfoSistema = NULL;
    /*<!Inicia JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
    try{
        try
        {
            validarUUID(DatosCfdi.CfdiComoCadena,DatosCfdi.strXmlDoc64);
            int cuantos = TSeleccionarPacParaCancelacion::CuantosThreadHay();
            _InformationalLog(strLog+"Cuantos threads ocupados hay:  "+UnicodeString(cuantos), G8_TOKEN_CANCELACION);
            if(cuantos >= StrToIntDef(Utileria::LeeIniConfig(NombreSeccionGeneral,NumThreadsMaximo, IntToStr(ValorDefectoNumThreadsMaximo)), ValorDefectoNumThreadsMaximo))
                throw ExceptionServidor(ErrorCancelacionNoActiva, MODULO_CFDIS);
            if((DatosCfdi.CfdiComoCadena.Pos(EsquemaCancelRetencion)) && (Utileria::LeeIniConfig(NombreSeccionGeneral, NombreLlaveActivarCancelacionesRtc, L"1") == L"0")  )
                throw ExceptionServidor(ErrorCancelacionRtcNoActivaRtc, MODULO_CFDIS);

            if ( Utileria::LeeIniConfig(NombreSeccionGeneral, NombreLlaveActivarCancelaciones, L"1") == L"0" )
                throw ExceptionServidor(ErrorCancelacionNoActiva, MODULO_CFDIS);

            if (!ValidaSistema(UpperCase(strSistemaParaCancelacion) , iReinstalableParaCancelacion))
                throw ExceptionServidor(ErrorCancelacionNoActiva, MODULO_CFDIS);

            _IniciaTicks;

            ValidacionesPreviasCancelaciones(DatosCfdi.CfdiComoCadena,Info,DatosCfdi);
            TseleccionarApuntadoresBD::obtenerBD(DatosCfdi.IdEmisor,objDatosConexion);
            _InformationalLog(strLog+"conexion a la base: "+objDatosConexion.nombreBD+" host: "+objDatosConexion.Host, G8_TOKEN_CANCELACION);
            obtenerOComprometerCancelacion(DatosCfdi.CfdiComoCadena, Info, DatosCfdi, CfdiComprometido,objDatosConexion);
            ComprometePaqueteCancelaciones(DatosCfdi,PaqueteComprometido);
            _InformationalLog(strLog+"Iniciando proceso interno de cancelación "+strEmisor, G8_TOKEN_CANCELACION);
            bool HuboTimeout = false;
            _InformationalLog(strLog+"Ejecutara Cancela "+strEmisor+" estado: "+IntToStr(DatosCfdi.Estado), G8_TOKEN_CANCELACION);
            if(DatosCfdi.Estado != EstCfdiTimbrado)
            {
                try
                {
                    Cancela(DatosCfdi, Resultado, MensajeResultado, CodigoResultado, Acuse,objDatosConexion);
                    DatosCfdi.Acuse = Acuse;
                }
                catch(ESOAPHTTPException &e)
                {
                    if ( e.StatusCode == ERROR_INTERNET_TIMEOUT )
                    {
                        _NoticeLog(strLog+"El PAC con id="+IntToStr(DatosCfdi.IdPAC)+" no respondio a la peticion de timbrado para  CFDI id="+IntToStr(DatosCfdi.Id), G8_TOKEN_CANCELACION);
                        HuboTimeout = true;
                        MensajeResultado = ErrorUUIDVacio;
                        CodigoResultado = ERROR_UUID_VACIO;
                    }
                    else
                        throw Exception(e.Message,e.HelpContext);
                }

                TEstadoCfdi NuevoEstado = HuboTimeout ? EstCfdiPendiente : EstCfdiTimbrado;
                TOperacionPaquete OpPaquete = DatosCfdi.Estado == EstCfdiPendiente || DatosCfdi.Estado == EstNecesitaAutorizacion ? OpPaqLiberar : OpPaqDecrementarYLiberar;
                DatosCfdi.DuracionProceso = _TiempoTotal;
                DatosCfdi.Sistema =  UpperCase(strSistemaParaCancelacion) + ".EXE";
                _DebugLog(strLog+"Cencelado exitoso se procede a Actualizar con los siguientes datos: opPaquete= "+IntToStr(OpPaquete)+" opCfdi= 3(Actualizar) NuevoEstado= "+IntToStr(NuevoEstado), G8_TOKEN_CANCELACION);
                Actualiza(DatosCfdi, NuevoEstado, OpCfdiActualizaCancelado, OpPaquete,objDatosConexion);
                _InformationalLog(strLog+"Fin del proceso de cancelacion exitosamente con el nuevo esquema de cancelacion: Resultado="+Utileria::ErrorCodigoMensaje(MensajeResultado, CodigoResultado, DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError), G8_TOKEN_CANCELACION);
            } else{
                CodigoResultado=201;
                MensajeResultado="CFDI Cancelado";
                _InformationalLog(strLog+"Fin del proceso porque esta en estado 4 y ya existe el Acuse: Resultado= "+Utileria::ErrorCodigoMensaje(MensajeResultado, CodigoResultado, DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError), G8_TOKEN_CANCELACION);
            }
            PaqueteComprometido = false;
            _InformationalLog(strLog+"keyCancel Sistema= "+strSistemaParaCancelacion+" Reinstalable= "+IntToStr(iReinstalableParaCancelacion)+" RFCEmisor= "+DatosCfdi.RfcEmisor+" Servidor= "+ Utileria::LeeIniConfig(NombreSeccionGeneral,"Servidor","0"), G8_TOKEN_CANCELACION);
            _LogSalida(G8_TOKEN_CANCELACION);
            return Utileria::ErrorCodigoMensaje(MensajeResultado, CodigoResultado, DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError);
        }
        catch(Exception &e)
        {
            _LogExcepcion(G8_TOKEN_CANCELACION);

             _ErrorLog(strLog+"Inicio de catch principal de la funcion EjecutaCancelarCFDIV2: idEmisor="+IntToStr(DatosCfdi.IdEmisor), G8_TOKEN_CANCELACION);
               if (DatosCfdi.IdEmisor!=0) {
                    TseleccionarApuntadoresBD::obtenerBD(DatosCfdi.IdEmisor,objDatosConexion);
               _InformationalLog(strLog+"keyCancel Catch Principal Sistema= "+strSistemaParaCancelacion+" Reinstalable= "+IntToStr(iReinstalableParaCancelacion)+" RFCEmisor= "+DatosCfdi.RfcEmisor+" Servidor= "+ Utileria::LeeIniConfig(NombreSeccionGeneral,"Servidor","0")+ " | No cancelado codigoError= "+IntToStr(e.HelpContext), G8_TOKEN_CANCELACION);
               } else if (DatosCfdi.IdEmisor==0){
                strLog+=strCfdiCancelacion.SubString(strCfdiCancelacion.Pos("<UUID>")+6,strCfdiCancelacion.Pos("</UUID>")-(strCfdiCancelacion.Pos("<UUID>")+6))+" | ";
                _InformationalLog(strLog+"keyCancel Catch Principal Sistema= "+strSistemaParaCancelacion+" Reinstalable= "+IntToStr(iReinstalableParaCancelacion)+" RFCEmisor= "+DatosCfdi.RfcEmisor+" Servidor= "+ Utileria::LeeIniConfig(NombreSeccionGeneral,"Servidor","0")+" | No cancelado codigoError= "+IntToStr(e.HelpContext), G8_TOKEN_CANCELACION);
               }
            if(DatosCfdi.Cfdi==NULL)
                DatosCfdi.Cfdi = Utileria::dameNuevoDocXML(DatosCfdi.CfdiComoCadena);

            TOperacionCfdi OpCfdi = CfdiComprometido ? OpCfdiEliminarCancelado : OpCfdiNinguna;
            TOperacionPaquete OpPaquete = PaqueteComprometido ? OpPaqLiberar : OpPaqNinguna;

            TEstadoCfdi NuevoEstado= EstCfdiIndefinido;
            if (DatosCfdi.Estado==EstCfdiPendiente ) /*<! JRMC 10/09/2018 Se comenta validación para no devolver folio cuando el estado de cancelacion es 5 y no se concluyo la cancelación por ser rechazado || DatosCfdi.Estado==EstNecesitaAutorizacion)*/
                 OpPaquete=OpPaqIncrementar;


            if(e.HelpContext==ERROR_CANCELACION_ENPROCESO){
                 _ErrorLog(strLog+"En el catch principal  se procede a Actualizar con los siguientes datos por error en la cancelacion: ",G8_TOKEN_CANCELACION);
                 switch(DatosCfdi.Estado)
                  {
                    case EstCfdiNuevo:
                        _ErrorLog(strLog+"Estado= Nuevo("+IntToStr(DatosCfdi.Estado)+ ") Actualizar estado a: "+IntToStr(EstNecesitaAutorizacion)+" opCFDI: "+IntToStr(OpCfdiActualizaCancelado)+" opPaquete: "+IntToStr(OpPaqDecrementarYLiberar), G8_TOKEN_CANCELACION);
                        OpCfdi      = OpCfdiActualizaCancelado;
                        OpPaquete   = OpPaqDecrementarYLiberar;
                        NuevoEstado = EstNecesitaAutorizacion;
                        break;
                    case EstNecesitaAutorizacion:
                        _ErrorLog(strLog+"Estado= NecesitaAutorizacion("+IntToStr(DatosCfdi.Estado)+ ") Actualizar estado a: "+IntToStr(EstNecesitaAutorizacion)+" opCFDI: "+IntToStr(OpCfdiNinguna)+" opPaquete: "+IntToStr(OpPaqNinguna), G8_TOKEN_CANCELACION);
                         OpCfdi      = OpCfdiNinguna;
                         OpPaquete   = OpPaqNinguna;
                         NuevoEstado = EstNecesitaAutorizacion;
                         break;
                    case EstCfdiPendiente:
                        _ErrorLog(strLog+"Estado=Pendiente("+IntToStr(DatosCfdi.Estado)+ ") Actualizar estado a: "+IntToStr(EstNecesitaAutorizacion)+" opCFDI: "+IntToStr(OpCfdiActualizaCancelado)+" opPaquete: "+IntToStr(OpPaqNinguna), G8_TOKEN_CANCELACION);
                        OpCfdi      = OpCfdiActualizaCancelado;
                        OpPaquete   = OpPaqNinguna;
                        NuevoEstado = EstNecesitaAutorizacion;
                        break;
                 }
                  /*<!Inicia JRMC 29/11/2018 se agrega condición  por RQ-CFDI para construir URL dinamico para el servicio de Aceptacion/Rechazo cuando el error 911(Cancelación en proceso)*/
                  DatosCfdi.urlCancelacion= Utileria::LeeIniConfig("CancelacionesV2", "AceptaRechaza","");
                  if(!DatosCfdi.urlCancelacion.IsEmpty())
                    ArmarURL(strXmlDoc64,DatosCfdi.urlCancelacion);
                  /*<!Termina JRMC 29/11/2018 se agrega condición  por RQ-CFDI para construir URL dinamico para el servicio de Aceptacion/Rechazo cuando el error 911(Cancelación en proceso)*/
            }
            else{

                _ErrorLog(strLog+"En el catch principal  se procede a Actualizar con los siguientes datos por error en la cancelacion : opPaquete= "+IntToStr(OpPaquete)+" opCfdi= 4(Eliminar) NuevoEstado= "+IntToStr(NuevoEstado), G8_TOKEN_CANCELACION);
            }


            if (DatosCfdi.IdEmisor!=0)
                Actualiza(DatosCfdi, NuevoEstado, OpCfdi, OpPaquete,objDatosConexion);

            if (DatosCfdi.strCancelDetalleError.IsEmpty()) {
                 DatosCfdi.strCancelDetalleError=e.Message;
                 e.Message="CFDI No Cancelado";
            }

            //_ErrorLog(strLog+"Fin del servicio de cancelacion con le nuevo esquema: Resultado= "+Utileria::ErrorCodigoMensaje(e.Message, e.HelpContext,DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError,DatosCfdi.urlCancelacion), G8_TOKEN_CANCELACION);
           /*<! JAMM 23/04/2019 se agrega la url generada en el XMl de respuesta */
            return Utileria::ErrorCodigoMensaje(e.Message, e.HelpContext,DatosCfdi.Acuse,DatosCfdi.strCancelDetalleError, DatosCfdi.urlCancelacion);
        }
    }__finally{
       if(DatosCfdi.XmlInfo) {
           DatosCfdi.XmlInfo->Active=false;
           DatosCfdi.XmlInfo=NULL;
       }
       if(DatosCfdi.Cfdi) {
      	 DatosCfdi.Cfdi->Active=false;
         DatosCfdi.Cfdi=NULL;
       }
        if(Resultado) {
            Resultado->Active=false;
            Resultado= NULL;
        }
    }
    /*<!Termina JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
}
//--------------------------------------------------------------------------
/*!
* \brief      Función que permite obtener el UUID del documento de cancelación y del documento a base 64  y compararlo para verificar que el UUID a cancelar corresponde al documento
* \author     Programador A. Josue Rodrigo Martín Canto.
* \version    5.0.0.0
* \date       17-Agosto-2018
* @param[in]  strXmlCancel contiene el UUID en una estructura del documento de cancelar
* @param[in]  strXmlDoc64 contiene el CFDI del UUID a cancelar   codigicado a 64 bits
* @param[out]
*/
void Cfdis::validarUUID(UnicodeString strXmlCancel,UnicodeString strXmlDoc64)
{
	 UnicodeString Resultado,strUUIDxmlCancel,strUUIDxmlDoc64,strXmlDoc;
	 //
	_di_IXMLDocument xmlPadre = NULL;
	_di_IXMLNode NodoHijo = NULL;
    /*<!Inicia JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
    try {
         try{
            //obtener UUID cancelacion

            strXmlCancel=StringReplace(strXmlCancel, L"&lt;", L"<", TReplaceFlags() << rfReplaceAll);
            strXmlCancel=StringReplace(strXmlCancel, L"&gt;", L">", TReplaceFlags() << rfReplaceAll);
            _DebugLog(strLog+"Antes de convertir  strXmlCancel a XML", G8_TOKEN_CANCELACION);
            xmlPadre = Utileria::dameNuevoDocXML(strXmlCancel);
            _DebugLog(strLog+"Despues  de convertir strXmlCancel en XML ", G8_TOKEN_CANCELACION);
            NodoHijo = Utileria::BuscaNodo(xmlPadre->DocumentElement, L"Folios");
            strUUIDxmlCancel=NodoHijo->GetChildValue(UnicodeString("UUID"));
            _DebugLog(strLog+"Se obtubo del XML de cancelacion el UUID= "+strUUIDxmlCancel,G8_TOKEN_CANCELACION);
            //fin
            NodoHijo = NULL;
            xmlPadre = NULL;
            //Obtener UUID del docuemnto a 64 bits
            /*!< Inicio JRMC 30/07/2018 se modifica por CC-CFDI-592 para decodificar de base 64 de forma mas eficiente  con openSSL*/
            AnsiString xml64=strXmlDoc64;
            strXmlDoc=Utileria::UnicodeStringToBase64(xml64,false);
            /*!< Termina JRMC 30/07/2018 se modifica por CC-CFDI-592 para decodificar de base 64 de forma mas eficiente  con openSSL*/
            _DebugLog(strLog+"Antes de convertir strXmlDoc64 a XML", G8_TOKEN_CANCELACION);
            xmlPadre = Utileria::dameNuevoDocXML_UTF8(strXmlDoc);
            NodoHijo = Utileria::BuscaNodo(xmlPadre->DocumentElement, L"TimbreFiscalDigital");
            _DebugLog(strLog+"Despues de convertir a strXmlDoc64 a XML", G8_TOKEN_CANCELACION);
            strUUIDxmlDoc64=NodoHijo->Attributes[WideString("UUID")];
            _DebugLog(strLog+"Se obtubo del XML CFDI el UUID: "+strUUIDxmlDoc64,G8_TOKEN_CANCELACION);
            //fin de la obtención de UUDI del documento códificado a 64 bits

            if(strUUIDxmlDoc64 != strUUIDxmlCancel)
                 throw ExceptionServidor(ErrorCancelUUIDiferente,MODULO_CFDIS);

           strLog+=strUUIDxmlCancel+" | ";
           }

            catch(Exception &e)
            {
                _ErrorLog(strLog+"Fin del catch de la funcion validarUUID: codigoError"+IntToStr(e.HelpContext),G8_TOKEN_CANCELACION);
                throw Exception(e.Message,e.HelpContext);
            }
        }__finally{
           if(xmlPadre) {
            xmlPadre->Active=false;
            xmlPadre=NULL;
            }
             NodoHijo= NULL;
        }
        /*<!Termina JRMC 10/12/2018 se agrega try finally por RQ-CFDI-844 para liberar siempre los objetos y no repetir código*/
 }
//--------------------------------------------------------------------------------------------------------

/*!
* \brief      Función que arma la URL para el usuario pueda aceptar o rechazar una cancelación
* \author     Programador A. Josue Rodrigo Martin Canto.
* \version    5.0.0.0
* \date       29-Noviembre-2018
* @param[in]  UnicodeString strXmlDoc64: XML timbrado en base64
* @param[in]  UnicodeString &strURL : String por referencia donde se guarda el URL generado.
* @param[out]
*/
void Cfdis::ArmarURL(UnicodeString strXmlDoc64, UnicodeString &strURL)
{
  UnicodeString serie,folio,monto,fecha,uuid,htp,strXmlDoc, rfcEmisor,rfcReceptor,url;
   _di_IXMLDocument xmlPadre = NULL;
   htp = strURL;
   strXmlDoc = Utileria::UnicodeStringToBase64( AnsiString (strXmlDoc64),false);
   xmlPadre = Utileria::dameNuevoDocXML_UTF8(strXmlDoc);

   try{ serie   = Raiz(xmlPadre)->Attributes["Serie"]; }catch(...) {}
   try{ folio   = Raiz(xmlPadre)->Attributes["Folio"]; }catch(...) {}
   try{ monto   = Raiz(xmlPadre)->Attributes["Total"]; }catch(...) {}
   try{ fecha   = Raiz(xmlPadre)->Attributes["Fecha"]; }catch(...) {}
   try{ rfcEmisor = Raiz(xmlPadre)->Hijo("Emisor")->Attributes[L"Rfc"]; }catch(...) {}
   try{ rfcReceptor =  Raiz(xmlPadre)->Hijo("Receptor")->Attributes[L"Rfc"]; }catch(...) {}
   try{ uuid = Utileria::BuscaNodo(xmlPadre->DocumentElement, L"TimbreFiscalDigital")->Attributes[WideString("UUID")]; }catch(...) {}

   url =  "UUID=" + uuid + "|RFCEmisor=" + rfcEmisor + "|RFCReceptor=" + rfcReceptor + "|Folio=" + folio + "|SERIE=" + serie + "|Monto=" + monto + "|Fecha=" + fecha;

   UnicodeString result;
   int iTamanio=(url.Length()+1)*2;
   char *pcDest=new char[iTamanio];
   wchar_t * buffXML=_wcsdup(url.c_str());
   try
   {
		UnicodeToUtf8(pcDest,buffXML,iTamanio);
		result=pcDest;
   }
   __finally
   {
		 delete[] pcDest;
		 free(buffXML);
         if(xmlPadre){
         xmlPadre->Active=false;
         xmlPadre=NULL;
         }
   }

	strURL = htp +"?token="+Utileria::UnicodeStringToBase64( result ,true);
    _DebugLog(strLog+"URL: "+strURL, G8_TOKEN_CANCELACION);
}

