





int Document::FunctionCall(const char *function_name,
						   ValueHash *context, 
						   ValueHash *parameters,
						   Value **value_ret,
						   const char **error_ret
						  )
{
	if (!strcmp(function_name,"reimpose")) {
		int result=Reimpose(dynamic_cast<Imposition *>(parameters->findObject("imposition")),
				 parameters->findBoolean("scalepages"),
				 parameters->findObject("papersize"),
				 error_ret
				);
	}
}



class Scriptable
{
 public:
	virtual StyleDef *getScriptStyleDef() = 0; //*** could return different defs depending on security settings??
	virtual int ContainsName(const char *fieldname, ScriptGlue **glue_ret) = 0; //0=no, 1=is var, 2=is method
	virtual int FunctionCall(const char *function_name,
					   ValueHash *context, 
					   ValueHash *parameters,
					   Value **value_ret,
					   const char **error_ret
					  ) = 0;
	 //try to map input Attribute to correctly labeled function parameters
	virtual int ParseParameters(Attribute *attparams, ValueHash **params_ret, char **error_ret);
};


......
class Document : ...., virtual public Scriptable {};

