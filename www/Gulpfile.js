var gulp = require("gulp"),
    less = require("gulp-less"),
    path = require("path");

gulp.task("less", function(){
    gulp.src("less/**/*.less")
        .pipe(less({
            paths: [
                path.join(__dirname, "less", "includes")
            ]
        }))
        .pipe(gulp.dest("css"));
});

gulp.task("default", [
    "less"
]);