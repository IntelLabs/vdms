import java.net.Socket;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;


public class SourceConnection extends WorkerConnection implements Runnable
{

    
    public SourceConnection(Socket newSocket)
    {
	super(newSocket);
    }

    @Override
    public void run()
    {

        //Send a vdms transaction with -number and payload

        // then do a read
	
    }

}
