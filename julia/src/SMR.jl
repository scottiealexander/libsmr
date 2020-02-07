module SMR

include("./SMRTypes.jl")

using .SMRTypes

export read_wavemark_channel, read_continuous_channel, read_event_channel,
       read_marker_channel, read_channel_info, get_channel_type, channel_string,
       get_read_function

include("./helpers.jl")

# ============================================================================ #
# global constant path to libsmr shared object file
const LIBSMR = @libpath("libsmr", @__DIR__, "..", "lib")

# ============================================================================ #
"""
`idx = get_channel_index(ifile, label)`
In:
    ifile - path to .smr file
    label - channel label

Out:
    idx - integer index of channel, -1 indicates failure
"""
function get_channel_index(ifile::String, label::String)::Int64
    return ccall((:channel_label_path_to_index, LIBSMR), Cint, (Cstring,
        Cstring), ifile, label)
end
# ============================================================================ #
"""
`wmrk = read_wavemark_channel(ifile::String, idx::Integer)` *OR*\n
`wmrk = read_wavemark_channel(ifile::String, label::String)`
### Input:
* ifile - the path to a .smr file
* idx - an integer channel index
* label - the channel label as a String

### Output:
* wmrk - a SMRWMrkChannel type with fields:\n
            timestamps: Nx1 Vector{Float64} of spike timestamps in seconds
            wavemarks: MxN Array{UInt8,2} of spike waveforms
            markers: 4xN Array{UInt8,2} of marker codes
"""
function read_wavemark_channel(ifile::String, idx::Integer)
    @calllib(ifile, idx, "wavemark", SMRWMrkChannel)
end
function read_wavemark_channel(ifile::String, label::String)
    return read_wavemark_channel(ifile, get_channel_index(ifile, label))
end
# ============================================================================ #
"""
`cont = read_continuous_channel(ifile::String, idx::Integer)` *OR*\n
`cont = read_continuous_channel(ifile::String, label::String)`
### Input:
 * see `read_wavemark_channel`

### Output:
* cont - a SMRContChannel type with fields:\n
            data: Nx1 Vector{Int16} of voltage values
            sampling_rate: channel sampling rate in Hz
"""
function read_continuous_channel(ifile::String, idx::Integer)
    @calllib(ifile, idx, "continuous", SMRContChannel)
end
function read_continuous_channel(ifile::String, label::String)
    return read_continuous_channel(ifile, get_channel_index(ifile, label))
end
# ============================================================================ #
"""
`evt = read_event_channel(ifile::String, idx::Integer)` *OR*\n
`evt = read_event_channel(ifile::String, label::String)`
### Input:
 * see `read_wavemark_channel`

### Output:
* evt - a SMREventChannel type with fields:\n
            data: Nx1 Vector{Float64} of event timestamps
"""
function read_event_channel(ifile::String, idx::Integer)
    @calllib(ifile, idx, "event", SMREventChannel)
end
function read_event_channel(ifile::String, label::String)
    return read_event_channel(ifile, get_channel_index(ifile, label))
end
# ============================================================================ #
"""
`mrk = read_marker_channel(ifile::String, idx::Integer)` *OR*\n
`mrk = read_marker_channel(ifile::String, label::String)`
### Input:
 * see `read_wavemark_channel`

### Output:
* mrk - a SMRMarkerChannel type with fields:\n
            timestamps: Nx1 Vector{Float64} of marker timestamps in seconds
            markers: 4xN Array{UInt8,2} of marker codes
            text: an Nx1 Vector{String} of text info for each marker
"""
function read_marker_channel(ifile::String, idx::Integer)
    @calllib(ifile, idx, "marker", SMRMarkerChannel)
end
function read_marker_channel(ifile::String, label::String)
    return read_marker_channel(ifile, get_channel_index(ifile, label))
end
# ============================================================================ #
"""
`ifo = read_channel_info(ifile)`
### Input:
* ifile - the path to a .smr file

### Output:
* ifo - a Vector{SMRChannelInfo} with one element per channel, try `display(ifo)` to see channel infomration
"""
function read_channel_info(ifile::String)

    ptr = ccall((:read_channel_array, LIBSMR), Ptr{cSMRChannelInfoArray}, (Cstring,), ifile)

    if ptr != C_NULL
        err = false
        ifo_array = unsafe_load(ptr)
        if ifo_array.ifo != C_NULL
            ifo = Array{SMRChannelInfo}(undef, ifo_array.length)

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

        ccall((:free_channel_info_array, LIBSMR), Cvoid, (Ptr{cSMRChannelInfoArray},), ptr)

        if err
            error("failed to load array of channel info structs")
        end
    else
        error("call to read_channel_array failed")
    end

    return ifo
end
# ============================================================================ #
function get_channel_type(ifile::String, label::String)
    return get_channel_type(read_channel_info(ifile), label)
end
# ============================================================================ #
function get_channel_type(ifo::Array{T}, label::String) where T<:SMRChannelInfo
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
# ============================================================================ #
function get_read_function(ifile::String, label::String)
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
# ============================================================================ #
function test_all(ifile::String="../b1_con_006.smr")

    ifo = read_channel_info(ifile)

    for chan in ifo
        str = channel_string(chan.kind)
        f = get_read_function(ifile, chan.title)
        print("Reading ", chan, "... ")
        x = f(ifile, chan.title)
        println("success")
    end

    return ifo
end
# ============================================================================ #
end #END MODULE
