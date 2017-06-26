
library(dplyr)
library(ggplot2)
library(scales)
library(gtools)

generate.test.cases <- function(image.sizes, pixel.sizes, tile.sizes) {
    df <- expand.grid(image.sizes,pixel.sizes,tile.sizes)
    colnames(df) <- c("image.size", "pixel.size", "tile.size")
    sizes <- paste(df$image.size, "×", df$image.size, sep="")
    df$image.name <- factor(sizes, mixedsort(levels(factor(sizes))))
    mixedsort(df$image.name)
    bits <- paste(df$pixel.size*8, "bit", sep="-")
    df$pixel.name <- factor(bits, mixedsort(levels(factor(bits))))

    df
}

tile.test.cases <- function() {
    image.sizes <- 4096 * 2^seq(0,5)
    pixel.sizes <- 2^seq(0,4)
    tile.sizes <- seq(16,2064,16)

    df <- generate.test.cases(image.sizes, pixel.sizes, tile.sizes)

    df$tile.count <- ceiling(df$image.size / df$tile.size)^2
    df$tile.space <- df$tile.count * (df$tile.size^2) * df$pixel.size
    # 16 = TIFF header, 272 = IFD size without tileoffsets+sizes
    df$ifd.space <- (df$tile.count * 8 * 2) + 272 + 16
    df$file.space <- df$tile.space + df$ifd.space

    df
}

strip.test.cases <- function() {
    image.sizes <- 4096 * 2^seq(0,5)
    pixel.sizes <- 2^seq(0,4)
    tile.sizes <- seq(1,257,1)

    df <- generate.test.cases(image.sizes, pixel.sizes, tile.sizes)
    df$strip.count <- ceiling(df$image.size / df$tile.size)
    df$strip.space <- df$strip.count * (df$tile.size * df$image.size) * df$pixel.size
    # 16 = TIFF header, 272 = IFD size without stripoffsets+sizes
    df$ifd.space <- (df$strip.count * 8 * 2) + 272 + 16
    df$file.space <- df$strip.space + df$ifd.space

    df
}

limit.min <- function(x, span) {
    tx <- log10(x)
    mid <- (min(tx) + max(tx)) / 2

    10^(mid - (span/2))
}

limit.max <- function(x, span) {
    tx <- log10(x)
    mid <- (min(tx) + max(tx)) / 2

    10^(mid + (span/2))
}

standard.plot.properties <- function(title, xlab, ylab, tilesize, nybreaks) {
    list(labs(title=title,
              x=xlab,
              y=ylab),
         annotation_logticks(sides="l", size=0.2,
                             short = unit(1/40, "inches"),
                             mid = unit(2/40, "inches"),
                             long = unit(3/40, "inches")),
         facet_wrap(~image.name, scales="free_y", ncol=2),
         theme_bw(base_size = 8),
         theme(plot.title = element_text(hjust = 0.5),
               panel.grid.major = element_blank(),
               panel.grid.minor = element_blank()),
         scale_x_continuous(breaks=seq(0, tilesize, tilesize/4),
                            minor_breaks=seq(0, tilesize, tilesize/16)),
         geom_step(size=0.2),
         scale_y_continuous(trans = 'log10',
                            breaks = trans_breaks('log10',
                                                  n=nybreaks, function(x) 10^x),
                            labels = trans_format('log10',
                                                  math_format(10^.x)))
         )
}

pixel.size.colours <- function() {
    list(scale_colour_brewer("Pixel size", palette = "Dark2"))
}

figure.tilesizes <- function() {
    tiles <- tile.test.cases()

    values <- function(x) { x / (1024 * 1024) }
    lims <- group_by(tiles, image.name) %>%
        summarise(min = limit.min(values(file.space), 2),
                  max = limit.max(values(file.space), 2))

    p <- ggplot(tiles, aes(y = values(file.space),
                           x=tile.size,
                           colour=pixel.name)) +
        standard.plot.properties("Tiled image file size",
                                 "Tile size (pixels)",
                                 "File size (MiB)",
                                 2048,
                                 3) +
        pixel.size.colours() +
        geom_hline(data=lims, aes(yintercept=min), alpha=0) +
        geom_hline(data=lims, aes(yintercept=max), alpha=0)

    print(p)
    ggsave(filename="tile-file-size.svg",
           plot=p, width=6, height=4)
}

figure.tilesizes()

figure.tilecounts <- function() {
    tiles <- tile.test.cases()

    tiles <- filter(tiles, pixel.name == "8-bit")
    p <- ggplot(tiles, aes(y = tile.count,
                           x=tile.size)) +
        standard.plot.properties("Tile count",
                                 "Tile size (pixels)",
                                 "Tile count",
                                 2048,
                                 2)

    print(p)
    ggsave(filename="tile-count.svg",
           plot=p, width=6, height=4)
}

figure.tilecounts()

figure.tiletime <- function() {
    tiles <- tile.test.cases()

    values <- function(space, count) { ((space / (1024^2)) * 0.22) + (count * 0.01) }
    lims <- group_by(tiles, image.name) %>%
        summarise(min = limit.min(values(file.space, tile.count), 2),
                  max = limit.max(values(file.space, tile.count), 2))

    p <- ggplot(tiles, aes(y = values(file.space, tile.count),
                           x=tile.size,
                           colour=pixel.name)) +
        standard.plot.properties("Tiled image file write time",
                                 "Tile size (pixels)",
                                 "Write time (ms)",
                                 2048,
                                 3) +
        pixel.size.colours() +
        geom_hline(data=lims, aes(yintercept=min), alpha=0) +
        geom_hline(data=lims, aes(yintercept=max), alpha=0)

    print(p)
    ggsave(filename="tile-write-time.svg",
           plot=p, width=6, height=4)
}

figure.tiletime()

figure.stripsizes <- function() {
    strips <- strip.test.cases()

    values <- function(x) { x / (1024 * 1024) }
    lims <- group_by(strips, image.name) %>%
        summarise(min = limit.min(values(file.space), 2),
                  max = limit.max(values(file.space), 2))

    p <- ggplot(strips, aes(y = values(file.space),
                            x=tile.size,
                            colour=pixel.name)) +
        standard.plot.properties("Stripped image file size",
                                 "Strip size (pixels)",
                                 "File size (MiB)",
                                 256,
                                 3) +
        pixel.size.colours() +
        geom_hline(data=lims, aes(yintercept=min), alpha=0) +
        geom_hline(data=lims, aes(yintercept=max), alpha=0)

    print(p)
    ggsave(filename="strip-file-size.svg",
           plot=p, width=6, height=4)
}

figure.stripsizes()

figure.stripcounts <- function() {
    strips <- strip.test.cases()

    strips <- filter(strips, pixel.name == "8-bit")
    p <- ggplot(strips, aes(y = strip.count,
                           x=tile.size)) +
        standard.plot.properties("Strip count",
                                 "Strip size (pixels)",
                                 "Strip count",
                                 256,
                                 4)

    print(p)
    ggsave(filename="strip-count.svg",
           plot=p, width=6, height=4)
}

figure.stripcounts()

figure.striptime <- function() {
    strips <- strip.test.cases()

    values <- function(space, count) { ((space / (1024^2)) * 2) + (count * 0) }
    lims <- group_by(strips, image.name) %>%
        summarise(min = limit.min(values(file.space, strip.count), 2),
                  max = limit.max(values(file.space, strip.count), 2))

    p <- ggplot(strips, aes(y = values(file.space, strip.count),
                            x=tile.size,
                            colour=pixel.name)) +
        standard.plot.properties("Stripped image write time",
                                 "Strip size (pixels)",
                                 "Write time (ms)",
                                 256,
                                 3) +
        pixel.size.colours() +
        geom_hline(data=lims, aes(yintercept=min), alpha=0) +
        geom_hline(data=lims, aes(yintercept=max), alpha=0)

    print(p)
    ggsave(filename="strip-write-time.svg",
           plot=p, width=6, height=4)
}

figure.striptime()
