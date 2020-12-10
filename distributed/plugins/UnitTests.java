class UnitTests 
{
    public static void main(String[] args)
    {
        String connectionString = "{\"RegistrationId\" : 1, \"FunctionId\" : 2 , \"HostName\" : \"localhost\", \"HostPort\" : 55550, \"InitBytes\" : \"ff\"}"; 

        String connectionStringArray = "[{\"RegistrationId\" : 1, \"FunctionId\" : 2 , \"HostName\" : \"localhost\", \"HostPort\" : 55550, \"InitBytes\" : \"ff\"}, {\"RegistrationId\" : 1, \"FunctionId\" : 2 , \"HostName\" : \"localhost\", \"HostPort\" : 55550, \"InitBytes\" : \"ff\"}]"; 

        VdmsConnection testConnection = new TcpVdmsConnection(connectionString);
        int testFunctionId = testConnection.GetFunctionId();

        
    }
}