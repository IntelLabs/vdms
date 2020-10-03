import java.util.ArrayList;
import java.util.List;

class VdmsTransaction
{
    byte size[];
    byte buffer[];
    int id;
    
    public VdmsTransaction(byte[] nSize, byte[] nBuffer)
    {
        size = nSize;
        buffer = nBuffer;
        id = -1;
    }

    public byte[] GetSize()
    {
    	return size;
    }

    public byte[] GetBuffer()
    {
	    return buffer;
    }

    public int GetId()
    {
        return id;
    }

    public void SetId(int nId)
    {
        id = nId;
    }

}
