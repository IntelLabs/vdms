import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

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
            out.write(ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(outMessage.GetId()).array());
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
        VdmsTransaction readValue = null;
        //
        try
        {
            in.read(readSizeArray, 0, 4);
            readSize = ByteBuffer.wrap(readSizeArray).order(ByteOrder.LITTLE_ENDIAN).getInt();
            //now i can read the rest of the data
            //System.out.println("publisher readsizearray - " + Arrays.toString(readSizeArray));		
            
            byte[] buffer = new byte[readSize];
            int totalReadSize = 0;
            while(totalReadSize < readSize)
            {
                int actualReadSize = in.read(buffer, 0, readSize-totalReadSize);
                totalReadSize += actualReadSize;
            }
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
            byte[] threadIdBuffer = new byte[4];
            in.read(threadIdBuffer, 0, 4);
            nId = ByteBuffer.wrap(threadIdBuffer).order(ByteOrder.LITTLE_ENDIAN).getInt();
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

