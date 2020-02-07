module SMRTypes

import Base: show

export cSMRWMrkChannel, SMRWMrkChannel, cSMRContChannel, SMRContChannel,
       cSMREventChannel, SMREventChannel, cSMRMarkerChannel, SMRMarkerChannel,
       cSMRChannelInfo, cSMRChannelInfoArray, SMRChannelInfo, show,
       channel_string

const MARKER_SIZE = UInt8(4)

abstract type SMRCType end
abstract type SMRType end

# =========================================================================== #
struct cSMRWMrkChannel <: SMRCType
    length::UInt64
    npt::UInt64
    timestamps::Ptr{Float64}
    markers::Ptr{UInt8}
    wavemarks::Ptr{Int16}
end

mutable struct SMRWMrkChannel <: SMRType
    timestamps::Vector{Float64}
    markers::Matrix{UInt8}
    wavemarks::Matrix{Int16}

    function SMRWMrkChannel(x::cSMRWMrkChannel)
        self = new()

        self.timestamps = Vector{Float64}(undef, x.length)
        copyto!(self.timestamps, unsafe_wrap(Vector{Float64}, x.timestamps, x.length, own=false))

        self.markers = Matrix{UInt8}(undef, MARKER_SIZE, x.length)
        copyto!(self.markers, unsafe_wrap(Vector{UInt8}, x.markers, x.length * MARKER_SIZE, own=false))

        self.wavemarks = Matrix{Int16}(undef, x.npt, x.length)
        copyto!(self.wavemarks, unsafe_wrap(Vector{Int16}, x.wavemarks, x.length * x.npt, own=false))

        return self
    end
end
# =========================================================================== #
struct cSMRContChannel <: SMRCType
    length::UInt64
    sampling_rate::Float64
    data::Ptr{Int16}
end

mutable struct SMRContChannel <: SMRType
    data::Vector{Int16}
    sampling_rate::Float64

    function SMRContChannel(x::cSMRContChannel)
        self = new()

        self.data = Vector{Int16}(undef, x.length)
        copyto!(self.data, unsafe_wrap(Vector{Int16}, x.data, x.length, own=false))

        self.sampling_rate = x.sampling_rate

        return self
    end
end
# =========================================================================== #
struct cSMREventChannel <: SMRCType
    length::UInt64
    data::Ptr{Float64}
end

mutable struct SMREventChannel <: SMRType
    data::Vector{Float64}

    function SMREventChannel(x::cSMREventChannel)
        self = new()

        self.data = Vector{Float64}(undef, x.length)
        copyto!(self.data, unsafe_wrap(Vector{Float64}, x.data, x.length, own=false))

        return self
    end
end
# =========================================================================== #
struct cSMRMarkerChannel <: SMRCType
    length::UInt64
    npt::UInt64
    timestamps::Ptr{Float64}
    markers::Ptr{UInt8}
    text::Ptr{UInt8}
end

mutable struct SMRMarkerChannel <: SMRType
    timestamps::Vector{Float64}
    markers::Matrix{UInt8}
    text::Vector{String}

    function SMRMarkerChannel(mrk::cSMRMarkerChannel)
        self = new()

        self.timestamps = Vector{Float64}(undef, mrk.length)
        copyto!(self.timestamps, unsafe_wrap(Vector{Float64}, mrk.timestamps, mrk.length, own=false))

        self.markers = Matrix{UInt8}(undef, MARKER_SIZE, mrk.length)
        copyto!(self.markers, unsafe_wrap(Vector{UInt8}, mrk.markers, mrk.length * MARKER_SIZE, own=false))

        self.text = Vector{String}(undef, mrk.length)

        ary = unsafe_wrap(Vector{UInt8}, mrk.text, mrk.length * mrk.npt, own=false)
        for k = 1:mrk.length
            isrt = ((k-1)*mrk.npt)+1
            iend = k*mrk.npt
            if all(x->x=='\0', ary[isrt:iend])
                self.text[k] = ""
            else
                self.text[k] = strip(join(map(Char, ary[isrt:iend])), '\0')
            end
        end

        return self
    end
end
# =========================================================================== #
struct cSMRChannelInfo <: SMRCType
    title::Cstring
    index::Int32
    kind::UInt8
    phy_chan::Int16
end

struct cSMRChannelInfoArray <: SMRCType
    length::UInt32
    ifo::Ptr{Ptr{cSMRChannelInfo}}
end

mutable struct SMRChannelInfo <: SMRType
    title::String
    index::Int
    kind::Int
    phy_chan::Int

    function SMRChannelInfo(x::cSMRChannelInfo)
        self = new()
        self.title = unsafe_string(x.title)
        self.index = Int(x.index)
        self.kind = Int(x.kind)
        self.phy_chan = Int(x.phy_chan)
        return self
    end
end
# =========================================================================== #
function show(io::IO, ifo::SMRChannelInfo)
    str = "\"" * ifo.title *
          "\": " * channel_string(ifo.kind) *
          ", " * string(ifo.index)
    print(io, str)
end
# =========================================================================== #
function channel_string(x::T) where T<:Integer
    sx = string(x)
    if x == 1
        str = "continuous"
    elseif x == 2
        str = "event"
    elseif x == 3
        str = "event"
    elseif x == 4
        str = "event"
    elseif x == 5
        str = "marker"
    elseif x == 6
        str = "adc_marker"
    elseif x == 7
        str = "real_marker"
    elseif x == 8
        str = "text_marker"
    elseif x == 9
        str = "real_wave"
    else
        @warn(string(x) * " is not a valid channel type")
        str = "INVALID"
    end
    return str
end
# =========================================================================== #
end #END MODULE
