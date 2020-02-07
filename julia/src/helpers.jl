# ============================================================================ #
macro libpath(prefix, dir...)
    @static if Sys.islinux()
        ext = ".so"
    elseif Sys.iswindows()
        ext = ".dll"
    elseif Sys.isapple()
        # nothing to do here
        ext = ""
    else
        ext = ".so"
    end

    return quote
        # path to lib is now relative to the containing file
        # (i.e. the file that is invoking this macro)
        joinpath($(dir...), $prefix * $ext)
    end
end
# ============================================================================ #
macro calllib(ifile, idx, chan_type, jtyp)#ctyp, jtyp)

    ctyp = Symbol("c", jtyp)
    # jtyp = Symbol("SMR" * type_name * "Channel")

    fread = "read_" * chan_type * "_channel"
    ffree = "free_" * chan_type * "_channel"

    return quote
        if splitext($(esc(ifile)))[2] != ".smr"
            error("Input file is not an smr file")
        end

        ptr = ccall(($fread, LIBSMR), Ptr{$ctyp}, (Cstring, Cint),
            $(esc(ifile)), Cint($(esc(idx))))

        if ptr != C_NULL
            out = $jtyp(unsafe_load(ptr))
            ccall(($ffree, LIBSMR), Cvoid, (Ptr{$ctyp},), ptr)
        else
            error("call to " * string($fread) * " failed")
        end
        out
    end
end
# ============================================================================ #
