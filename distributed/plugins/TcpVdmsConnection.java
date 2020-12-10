import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

class TcpVdmsConnection extends VdmsConnection 
{
    Socket socket;
    DataInputStream in;
    DataOutputStream out;
    
    public TcpVdmsConnection(String initString)
    {
        super(initString);
        try
        {
            socket = new Socket(hostName, hostPort);
            in = new DataInputStream(socket.getInputStream());
            out = new DataOutputStream(socket.getOutputStream());
        }
        catch(UnknownHostException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
        catch(IOException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    public void Close()
    {
        try 
        {
            in.close();
            out.close();
            socket.close();
        }
        catch(IOException e)
        {
            e.printStackTrace();
            System.out.println("Socket was closed\n");
        }        
    }
    
    public void WriteInitMessage()
    {
        try
        {
            out.write(initSequence.GetSize());
            out.write(initSequence.GetBuffer());
        }
        catch(IOException e)
        {
            e.printStackTrace();
            System.exit(-1); 
        }
    }
    
    public void Write(VdmsTransaction outMessage)
    {
        try
        {    
            out.write(outMessage.GetSize());
            out.write(outMessage.GetBuffer());
        }
        catch(IOException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }
    
    public void WriteExtended(VdmsTransaction outMessage)
    {
        Write(outMessage);
        try
        {
            out.write(ByteBuffer.allocate(4).putInt(outMessage.GetId()).array());
        }
        catch(IOException e)
        {
            e.printStackTrace();
            System.exit(-1);
        } 
    }
    
    public VdmsTransaction Read()
    {
        byte[] readSizeArray = new byte[4];
        int readSize;
        int int0, int1, int2, int3;
        VdmsTransaction readValue = null;
        //
        try
        {
            in.read(readSizeArray, 0, 4);
            int0 = (byte) (readSizeArray[0] & 255);
            int1 = (byte) (readSizeArray[1] & 255);
            int2 = (byte) (readSizeArray[2] & 255);
            int3 = (byte) (readSizeArray[3] & 255);
            readSize = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
            //now i can read the rest of the data
            //System.out.println("publisher readsizearray - " + Arrays.toString(readSizeArray));		
            
            byte[] buffer = new byte[readSize];
            in.read(buffer, 0, readSize);
            readValue = new VdmsTransaction(readSizeArray, buffer);
            
        }
        
        catch(IOException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
        
        return readValue;
    }
    
    public VdmsTransaction ReadExtended()
    {  
        VdmsTransaction readValue = Read();
        try
        {
            int nId;
            int int0, int1, int2, int3;
            byte[] threadIdBuffer = new byte[4];
            in.read(threadIdBuffer, 0, 4);
            int0 = (byte) (threadIdBuffer[0] & 255);
            int1 = (byte) (threadIdBuffer[1] & 255);
            int2 = (byte) (threadIdBuffer[2] & 255);
            int3 = (byte) (threadIdBuffer[3] & 255);
            nId = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
            readValue.SetId(nId);
        }
        catch(IOException e)
        {
            e.printStackTrace();
            System.exit(-1);
            
        }
        return readValue;
    }
    
    
    
}

