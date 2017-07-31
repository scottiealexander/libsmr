module SMRTypes

import Base: show

export cSMRWMrkChannel, SMRWMrkChannel, cSMRContChannel, SMRContChannel,
       cSMREventChannel, SMREventChannel, cSMRMarkerChannel, SMRMarkerChannel,
       cSMRChannelInfo, cSMRChannelInfoArray, SMRChannelInfo, show,
       channel_string

const MARKER_SIZE = UInt8(4)

abstract SMRCType
abstract SMRType

# =========================================================================== #
immutable cSMRWMrkChannel <: SMRCType
    length::UInt64
    npt::UInt64
    timestamps::Ptr{Float64}
    markers::Ptr{UInt8}
    wavemarks::Ptr{Int16}
end

type SMRWMrkChannel <: SMRType
    timestamps::Array{Float64}
    markers::Array{UInt8}
    wavemarks::Array{Int16}

    function SMRWMrkChannel(x::cSMRWMrkChannel)
        self = new()

        self.timestamps = Array{Float64}(x.length)
        copy!(self.timestamps, pointer_to_array(x.timestamps, x.length, false))

        self.markers = Array{UInt8}(x.length * MARKER_SIZE)
        copy!(self.markers, pointer_to_array(x.markers, x.length * MARKER_SIZE, false))

        self.markers = reshape(self.markers, Int(MARKER_SIZE), Int(x.length))

        self.wavemarks = Array{Int16}(x.length * x.npt)
        copy!(self.wavemarks, pointer_to_array(x.wavemarks, x.length * x.npt, false))

        self.wavemarks = reshape(self.wavemarks, Int(x.npt), Int(x.length))

        return self;
    end
end
# =========================================================================== #
immutable cSMRContChannel <: SMRCType
    length::UInt64
    sampling_rate::Float64
    data::Ptr{Int16}
end

type SMRContChannel <: SMRType
    data::Array{Int16}
    sampling_rate::Float64

    function SMRContChannel(x::cSMRContChannel)
        self = new()

        self.data = Array{Int16}(x.length)
        copy!(self.data, pointer_to_array(x.data, x.length, false))

        self.sampling_rate = x.sampling_rate

        return self;
    end
end
# =========================================================================== #
immutable cSMREventChannel <: SMRCType
    length::UInt64
    data::Ptr{Float64}
end

type SMREventChannel <: SMRType
    data::Array{Float64}

    function SMREventChannel(x::cSMREventChannel)
        self = new()

        self.data = Array{Float64}(x.length)
        copy!(self.data, pointer_to_array(x.data, x.length, false))

        return self;
    end
end
# =========================================================================== #
immutable cSMRMarkerChannel <: SMRCType
    length::UInt64
    npt::UInt64
    timestamps::Ptr{Float64}
    markers::Ptr{UInt8}
    text::Ptr{UInt8}
end

type SMRMarkerChannel <: SMRType
    timestamps::Array{Float64}
    markers::Array{UInt8}
    text::Array{ASCIIString}

    function SMRMarkerChannel(mrk::cSMRMarkerChannel)
        self = new()

        self.timestamps = Array{Float64}(mrk.length)
        copy!(self.timestamps, pointer_to_array(mrk.timestamps, mrk.length, false))

        self.markers = Array{UInt8}(mrk.length * MARKER_SIZE)
        copy!(self.markers, pointer_to_array(mrk.markers, mrk.length * MARKER_SIZE, false))

        self.markers = reshape(self.markers, Int(MARKER_SIZE), Int(mrk.length))

        self.text = Array{ASCIIString}(mrk.length)

        ary = pointer_to_array(mrk.text, mrk.length * mrk.npt, false)
        for k = 1:mrk.length
            isrt = ((k-1)*mrk.npt)+1
            iend = k*mrk.npt
            if all(x->x=='\0', ary[isrt:iend])
                self.text[k] = ""
            else
                self.text[k] = strip(join(map(Char, ary[isrt:iend])), '\0')
            end
        end

        return self;
    end
end
# =========================================================================== #
immutable cSMRChannelInfo <: SMRCType
    title::Cstring
    index::Int32
    kind::UInt8
    phy_chan::Int16
end

immutable cSMRChannelInfoArray <: SMRCType
    length::UInt32
    ifo::Ptr{Ptr{cSMRChannelInfo}}
end

type SMRChannelInfo <: SMRType
    title::ByteString
    index::Int
    kind::Int
    phy_chan::Int

    function SMRChannelInfo(x::cSMRChannelInfo)
        self = new()
        self.title = bytestring(x.title)
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
function channel_string{T<:Integer}(x::T)
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
        warn(string(x) * " is not a valid channel type")
        str = "INVALID"
    end
    return str
end
# =========================================================================== #
end #END MODULE
