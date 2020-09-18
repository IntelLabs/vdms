import java.net.Socket;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class WorkerConnection
{
    Socket serverSocket;
    DataInputStream inStream;
    DataOutputStream outStream;
    public WorkerConnection(Socket newSocket)
    {
	serverSocket = newSocket;
	try
	    {
		inStream = new DataInputStream(serverSocket.getInputStream());
		outStream = new DataOutputStream(serverSocket.getOutputStream());
	    }
	catch(Exception e)
	    {
		e.printStackTrace();
	    }
    }

    public VdmsTransaction Publish(VdmsTransaction outMessage)
    {
	byte[] readSizeArray = new byte[4];
	byte[] buffer;
	try
	    {
		outStream.write(outMessage.GetSize());
		outStream.write(outMessage.GetBuffer());

		int readReturn = inStream.read(readSizeArray, 0, 4);
		int int0 = readSizeArray[0];
		int int1 = readSizeArray[1];
		int int2 = readSizeArray[2];
		int int3 = readSizeArray[3];
		int readSize = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
		buffer = new byte[readSize];
		readReturn = inStream.read(buffer, 0, readSize);
		System.out.println("returned from server " + Integer.toString(readSize));
		VdmsTransaction returnTransaction = new VdmsTransaction(readSizeArray, buffer);
		return returnTransaction;
	
	    }
	catch(IOException e)
	    {
		e.printStackTrace(); 

	    }
	return null;
    }
}


