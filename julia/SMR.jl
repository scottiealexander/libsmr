module SMR

using SMRTypes

export read_wavemark_channel, read_continuous_channel, read_event_channel,
       read_marker_channel, read_channel_info, get_channel_type, channel_string,
       get_read_function

# =========================================================================== #
macro libname(dir, prefix)
    os = lowercase(string(OS_NAME))
    str = "libsmr"
    if OS_NAME == :Linux
        str *= ".so"
    elseif OS_NAME == :Windows
        str *= ".dll"
    elseif OS_NAME == :Darwin
        # nothing to do here
    else
        str *= ".so"
    end

    # path to lib is now relative to the containing file
    current_dir = splitdir(@__FILE__)[1]
    ofile = joinpath(current_dir, "libsmr", os , str)
    return ofile
end
# =========================================================================== #
macro calllib(ifile, label, idx, fread, ffree, ctyp, jtyp)
    quote
        if splitext(ifile)[2] != ".smr"
            error("Input file is not an smr file")
        end
        if (idx < 0) && (!isempty(label))
            cidx = ccall((:channel_label_path_to_index, LIBSMR), Cint, (Cstring, Cstring), $ifile, $label)
            cidx < 0 && error("Failed to fetch channel index for channel: \"" * $label * "\"")
        elseif idx > 0
            cidx = idx
            if !isempty(label)
                warn("Channel index and label given, ignoring label!")
            end
        else
            error("No channel label or index given")
        end

        ptr = ccall(($fread, LIBSMR), Ptr{$ctyp}, (Cstring, Cint), $ifile, Cint(cidx))

        if ptr != C_NULL
            out = $jtyp(unsafe_load(ptr))
            ccall(($ffree, LIBSMR), Void, (Ptr{$ctyp},), ptr)
        else
            error("call to " * string($fread) * " failed")
        end
        out
    end
end
# =========================================================================== #
# global constant path to libsmr shared object file
const LIBSMR = @libname("lib","libsmr")
# =========================================================================== #
"""
`wmrk = read_wavemark_channel(ifile, idx)` *OR*\n
`wmrk = read_wavemark_channel(ifile, label=label)`
### Input:
* ifile - the path to a .smr file
* idx - an integer channel index
* [label] - the channel label as a ByteString

### Output:
* wmrk - a SMRWMrkChannel type with fields:\n
            timestamps: Nx1 Vector{Float64} of spike timestamps in seconds
            wavemarks: MxN Array{UInt8,2} of spike waveforms
            markers: 4xN Array{UInt8,2} of marker codes
"""
function read_wavemark_channel(ifile::ByteString, idx::Integer=-1; label::ByteString="")
    @calllib(ifile, label, idx, :read_wavemark_channel, :free_wavemark_channel, cSMRWMrkChannel, SMRWMrkChannel)
end
# =========================================================================== #
"""
`cont = read_continuous_channel(ifile, idx)` *OR*\n
`cont = read_continuous_channel(ifile, label=label)`
### Input:
 * see `read_wavemark_channel`

### Output:
* cont - a SMRContChannel type with fields:\n
            data: Nx1 Vector{Int16} of voltage values
            sampling_rate: channel sampling rate in Hz
"""
function read_continuous_channel(ifile::ByteString, idx::Integer=-1; label::ByteString="")
    @calllib(ifile, label, idx, :read_continuous_channel, :free_continuous_channel, cSMRContChannel, SMRContChannel)
end
# =========================================================================== #
"""
`evt = read_event_channel(ifile, idx)` *OR*\n
`evt = read_event_channel(ifile, label=label)`
### Input:
 * see `read_wavemark_channel`

### Output:
* evt - a SMREventChannel type with fields:\n
            data: Nx1 Vector{Float64} of event timestamps
"""
function read_event_channel(ifile::ByteString, idx::Integer=-1; label::ByteString="")
    @calllib(ifile, label, idx, :read_event_channel, :free_event_channel, cSMREventChannel, SMREventChannel)
end
# =========================================================================== #
"""
`mrk = read_marker_channel(ifile, idx)` *OR*\n
`mrk = read_marker_channel(ifile, label=label)`
### Input:
 * see `read_wavemark_channel`

### Output:
* mrk - a SMRMarkerChannel type with fields:\n
            timestamps: Nx1 Vector{Float64} of marker timestamps in seconds
            markers: 4xN Array{UInt8,2} of marker codes
            text: an Nx1 Vector{ASCIIString} of text info for each marker
"""
function read_marker_channel(ifile::ByteString, idx::Integer=-1; label::ByteString="")
    @calllib(ifile, label, idx, :read_marker_channel, :free_marker_channel, cSMRMarkerChannel, SMRMarkerChannel)
end
# =========================================================================== #
"""
`ifo = read_channel_info(ifile)`
### Input:
* ifile - the path to a .smr file

### Output:
* ifo - a Vector{SMRChannelInfo} with one element per channel, try `display(ifo)` to see channel infomration
"""
function read_channel_info(ifile::ByteString)

    ptr = ccall((:read_channel_array, LIBSMR), Ptr{cSMRChannelInfoArray}, (Cstring,), ifile)

    if ptr != C_NULL
        err = false
        ifo_array = unsafe_load(ptr)
        if ifo_array.ifo != C_NULL
            ifo = Array{SMRChannelInfo}(ifo_array.length)

            for k = 1:ifo_array.length
                tmp = unsafe_load(ifo_array.ifo ,k)
                if tmp != C_NULL
                    ifo[k] = SMRChannelInfo(unsafe_load(tmp))
                else
                    print("WARNING: Failed to read array element " * string(k))
                end
            end
        else
            err = true
        end

        ccall((:free_channel_info_array, LIBSMR), Void, (Ptr{cSMRChannelInfoArray},), ptr)

        if err
            error("failed to load array of channel info structs")
        end
    else
        error("call to read_channel_array failed")
    end

    return ifo
end
# =========================================================================== #
function get_channel_type(ifile::ByteString, label::ByteString)
    return get_channel_type(read_channel_info(ifile), label)
end
# =========================================================================== #
function get_channel_type{T<:SMRChannelInfo}(ifo::Array{T}, label::ByteString)
    tp = 0
    label = lowercase(label)

    for x in ifo
        if label == lowercase(x.title)
            tp = x.kind
            break
        end
    end
    return tp
end
# =========================================================================== #
"""
idx = get_channel_index(ifile, label)
In:
    ifile - path to .smr file
    label - channel label

Out:
    idx - integer index of channel, -1 indicates failure
"""
function get_channel_index(ifile::ByteString, label::ByteString)
    return ccall((:channel_label_path_to_index, LIBSMR), Cint, (Cstring, Cstring), ifile, label)
end
# =========================================================================== #
function get_read_function(ifile::ByteString, label::ByteString)
    typ = get_channel_type(ifile, label)
    if typ == 1
        f = read_continuous_channel
    elseif typ == 2
        f = read_event_channel
    elseif typ == 3
        f = read_event_channel
    elseif typ == 4
        f = read_event_channel
    elseif typ == 5
        f = read_marker_channel
    elseif typ == 6
        f = read_wavemark_channel
    elseif typ == 7
        f = read_marker_channel
    elseif typ == 8
        f = read_marker_channel
    elseif typ == 9
        error("real wave channels are not yet supported!")
    else
        error("Channel $(label) cannot be located")
    end
end
# =========================================================================== #
function test_all()
    ifile = "../b1_con_006.smr"

    print("Reading wavemark channel... ")
    x = read_wavemark_channel(ifile, label="WMrk 4")
    print("success\n")

    print("Reading continuous channel... ")
    x = read_continuous_channel(ifile, label="Cont 4")
    print("success\n")

    print("Reading event channel... ")
    x = read_event_channel(ifile, label="Stim")
    print("success\n")

    print("Reading marker channel... ")
    x = read_marker_channel(ifile, label="untitled")
    print("success\n")

    print("Reading channel info... ")
    x = read_channel_info(ifile)
    print("success\n")

    return x
end
# =========================================================================== #
end #END MODULE
